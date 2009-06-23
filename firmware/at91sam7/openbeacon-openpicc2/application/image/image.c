/*
 * image.c
 *
 *  Created on: 01.06.2009
 *      Author: henryk
 */

#include <FreeRTOS.h>
#include <AT91SAM7.h>

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "eink/eink.h"
#include "lzo/minilzo.h"
#include "dosfs/dosfs.h"
#include "dosfs/fat_helper.h"
#include "image/splash.h"

static int lzo_initialized = 0;

static unsigned char scratch_space[MAX_PART_SIZE] __attribute__((aligned (2), section(".sdram")));

static int image_lzo_init(void)
{
	if(lzo_init() != LZO_E_OK) {
		printf("internal error - lzo_init() failed !!!\n");
		printf("(this usually indicates a compiler bug - try recompiling\nwithout optimizations, and enable `-DLZO_DEBUG' for diagnostics)\n");
		return -ENOMEM;
	} else {
		printf("LZO real-time data compression library (v%s, %s).\n",
				lzo_version_string(), lzo_version_date());
		lzo_initialized = 1;
		return 0;
	}
}

/* Unpack a splash image into an image_t. The target must already have been assigned
 * its memory and have its data and size members set.
 */
int image_unpack_splash(image_t target, const struct splash_image * const source)
{
	const struct splash_part * const * image_parts = source->splash_parts;
	void *current_target = target->data;
	
	if(!lzo_initialized) {
		int r = image_lzo_init();
		if(r < 0) return r;
	}
	
	if(target->data == NULL) {
		return -EFAULT;
	}

	const struct splash_part * const * current_part = image_parts;
	unsigned int output_size = 0;
	
	while(*current_part != 0) {
		lzo_uint out_len = sizeof(scratch_space);
		if(lzo1x_decompress_safe((unsigned char*)((*current_part)->data), (*current_part)->compress_len, scratch_space, &out_len, NULL) == LZO_E_OK) {
			//printf(".");
		} else {
			return -EBADMSG;
		}
		
		if(output_size + out_len > target->size) {
			return -ENOMEM;
		}
		
		memcpy(current_target, scratch_space, out_len);
		current_target += out_len;
		output_size += out_len;
		current_part++;
	}
	
	target->size = output_size;
	target->width = source->width;
	target->height = source->height;
	target->bits_per_pixel = source->bits_per_pixel;
	target->rowstride = ROUND_UP(target->width*target->bits_per_pixel, 8) / 8;
	return 0;
}

/* Create (or fill) an image_t with a solid color.
 * The target must already have set its data, size and bits_per_pixel members, the
 * width, height and rowstride members will be set to default values if they are 0.
 */
int image_create_solid(image_t target, uint8_t color, int width, int height)
{
	if(target == NULL) return -EINVAL;
	if(target->data == NULL) {
		return -EFAULT;
	}
	
	if(target->width == 0) target->width = width;
	if(target->height == 0) target->height = height;
	if(target->rowstride == 0) target->rowstride = ROUND_UP(target->width*target->bits_per_pixel, 8) / 8;
	
	if((unsigned int) (target->rowstride * (height-1) + width) > target->size) {
		return -ENOMEM;
	}
	
	uint8_t byteval = 0;
	switch(target->bits_per_pixel) {
	case IMAGE_BPP_8: byteval = color; break;
	case IMAGE_BPP_4: byteval = (color & 0xF0) | (color >> 4); break;
	case IMAGE_BPP_2: byteval = (color & 0xC0) | ((color&0xC0) >> 2) | ((color&0xC0) >> 4) | ((color&0xC0) >> 6); break;
	}
	
	if(target->rowstride == width) {
		memset(target->data, byteval, ROUND_UP(width*height*target->bits_per_pixel, 8) / 8);
	} else {
		int i;
		for(i=0; i<height; i++) {
			memset(target->data + i*target->rowstride, byteval, ROUND_UP(width*target->bits_per_pixel, 8)/8);
		}
	}
	
	return 0;
}

enum eink_pack_mode image_get_bpp_as_pack_mode(const image_t in) {
	if(in == 0) return 0;
	switch(in->bits_per_pixel) {
	case IMAGE_BPP_2: return PACK_MODE_2BIT;
	case IMAGE_BPP_4: return PACK_MODE_4BIT;
	case IMAGE_BPP_8: return PACK_MODE_1BYTE;
	}
	return 0;
}

int image_acquire(image_t *target, int width, int height, int rowstride, enum image_bpp bits_per_pixel)
{
	if(!target) return -EINVAL;
	if(!rowstride) rowstride = (width*bits_per_pixel+7)/8;   // rowstride = ceil( width / pixels_per_byte )
	
	int size = rowstride*height;
	uint8_t *data = malloc(size);
	if(!data) return -ENOMEM;
	
	(*target)->width = width;
	(*target)->height = height;
	(*target)->rowstride = rowstride;
	(*target)->bits_per_pixel = bits_per_pixel;
	(*target)->size = size;
	memset( &((*target)->damage_region), 0, sizeof((*target)->damage_region) );
	(*target)->data = data;
	(*target)->flags.malloced = 1;
	
	return 1;
}

int image_release(image_t *target)
{
	if(!target) return -EINVAL;
	if( (*target)->flags.malloced ) free((*target)->data);
	(*target)->flags.malloced = 0;
	(*target)->data = NULL;
	
	return 1;
}


static void _get_display_size(enum eink_rotation_mode rotation, int *w, int *h)
{
	switch(rotation) {
	case ROTATION_MODE_0:
	case ROTATION_MODE_180:
		*w = EINK_CURRENT_DISPLAY_CONFIGURATION->hsize;
		*h = EINK_CURRENT_DISPLAY_CONFIGURATION->vsize;
		break;
	case ROTATION_MODE_90:
	case ROTATION_MODE_270:
		*w = EINK_CURRENT_DISPLAY_CONFIGURATION->vsize;
		*h = EINK_CURRENT_DISPLAY_CONFIGURATION->hsize;
		break;
	}
}

int image_load_image_buffer(image_in_image_buffer_t target, eink_image_buffer_t buffer, image_t source,
		enum eink_rotation_mode rotation_mode)
{
	return image_load_image_buffer_position(target, buffer, source, rotation_mode, 0, 0);
}

extern int image_load_image_buffer_centered(image_in_image_buffer_t target, eink_image_buffer_t buffer, image_t source,
		enum eink_rotation_mode rotation_mode);

int image_load_image_buffer_position(image_in_image_buffer_t target, eink_image_buffer_t buffer, image_t source,
		enum eink_rotation_mode rotation_mode, int x, int y)
{
	if(target == NULL) return -EINVAL;
	if(buffer == NULL) return -EINVAL;
	if(source == NULL) return -EINVAL;
	
	return image_load_image_buffer_cropped(target, buffer, source, rotation_mode, x, y, source->width, source->height);
}

int image_load_image_buffer_cropped(image_in_image_buffer_t target, eink_image_buffer_t buffer, image_t source,
		enum eink_rotation_mode rotation_mode, int x, int y, int w, int h)
{
	return image_load_image_buffer_offset(target, buffer, source, rotation_mode, x, y, 0, 0, w, h);
}

/* Download a sub-image from source of width w and height h at coordinates off_x, off_y (with respect to source)
 * into the image buffer buffer at coordinates x,y and store relevant information about this relationship
 * in target.
 */
int image_load_image_buffer_offset(image_in_image_buffer_t target, eink_image_buffer_t buffer, const image_t source,
		enum eink_rotation_mode rotation_mode, int x, int y, int w, int h, int off_x, int off_y)
{
	if(target == NULL) return -EINVAL;
	if(buffer == NULL) return -EINVAL;
	if(source == NULL) return -EINVAL;
	
	int display_w=0, display_h=0;
	_get_display_size(rotation_mode, &display_w, &display_h);

	if(off_x >= source->width) return -EINVAL;
	if(off_x < 0) return -EINVAL;
	if(off_y >= source->height) return -EINVAL;
	if(off_y < 0) return -EINVAL;
	
	if(x >= display_w) return -EINVAL;
	if(x < 0) return -EINVAL;
	if(y >= display_h) return -EINVAL;
	if(y < 0) return -EINVAL;

	if(w < 0) return -EINVAL;
	if(h < 0) return -EINVAL;
	
	if(off_x+w > source->width) return -EINVAL;
	if(off_y+h > source->height) return -EINVAL;
	
	if(x+w > display_w) return -EINVAL;
	if(y+h > display_h) return -EINVAL;
	
	/* FIXME: Implement */
	
	return -1;
}

extern int image_update_image_buffer(image_in_image_buffer_t target);

int image_get_pixel(const struct image * const image, int x, int y)
{
	/*
	 * return (image->data[y*image->rowstride + (x*image->bits_per_pixel)/8] >> ((x*bg_image->bits_per_pixel) % 8))<<(8-image->bits_per_pixel);
	 */
	switch(image->bits_per_pixel) {
	case IMAGE_BPP_2:
		return (image->data[y*image->rowstride + x/4] >> (x%4))<<6;
	case IMAGE_BPP_4:
		return (image->data[y*image->rowstride + x/2] >> (x%2))<<4;
	case IMAGE_BPP_8:
		return image->data[y*image->rowstride + x];
	}
	return 0;
}

static inline void image_set_pixel_2bpp(image_t image, int x, int y, uint8_t value)
{
	int mask, byteindex;
	mask = 0x3 << ((x*2)%8);
	byteindex = y*image->rowstride + (x/4);
	image->data[byteindex] = (image->data[byteindex] & ~mask) | ( ((value>>6)<<((x*2)%8)) & mask);
}

static inline void image_set_pixel_4bpp(image_t image, int x, int y, uint8_t value)
{
	int mask, byteindex;
	byteindex = y*image->rowstride + (x/2);
	if(x % 2 == 0) {
		mask = 0x0f;
		image->data[byteindex] = (image->data[byteindex] & ~mask) | ( (value>>4) & mask);
	} else {
		mask = 0xf0;
		image->data[byteindex] = (image->data[byteindex] & ~mask) | ( value & mask);
	}
}

static inline void image_set_pixel_8bpp(image_t image, int x, int y, uint8_t value)
{
	int byteindex;
	byteindex = y*image->rowstride + x;
	image->data[byteindex] = value;
}

void image_set_pixel(image_t image, int x, int y, uint8_t value)
{
	/*
	 * int mask = ((0xff>>(8-image->bits_per_pixel)))<<( (x*image->bits_per_pixel) % 8);
	 * int byteindex = y*image->rowstride + (x*image->bits_per_pixel)/8;
	 * image->data[byteindex] = (image->data[byteindex] & ~mask)
	 * 		| ( ((value >> (8-image->bits_per_pixel)) << ((x*image->bits_per_pixel) % 8)) & mask);
	 */
	switch(image->bits_per_pixel) {
	case IMAGE_BPP_2:
		image_set_pixel_2bpp(image, x, y, value);
		break;
	case IMAGE_BPP_4:
		image_set_pixel_4bpp(image, x, y, value);
		break;
	case IMAGE_BPP_8:
		image_set_pixel_8bpp(image, x, y, value);
		break;
	}
}

inline int image_draw_straight_line(image_t image, int x1, int y, int x2, int value)
{
	int i;
	switch(image->bits_per_pixel) {
	case IMAGE_BPP_2:
		for(i=x1; i<=x2; i++) image_set_pixel_2bpp(image, i, y, value);
		break;
	case IMAGE_BPP_4: {
			int start, end;
			if(x1%2 == 1) {
				image_set_pixel_4bpp(image, x1, y, value);
				start = x1+1;
			} else {
				start = x1;
			}
			if(x2%2 == 0) {
				image_set_pixel_4bpp(image, x2, y, value);
				end = x2-1;
			} else {
				end = x2;
			}
			if(start < end) {
				memset(image->data + image->rowstride*y + start/2, (value & 0xf0) | ((value&0xf0) >> 4), (end-start+1)/2 );
			}
		}
		break;
	case IMAGE_BPP_8:
		memset(image->data + image->rowstride*y + x1, value, x2-x1+1);
		break;
	}
	return 0;
}

int image_draw_circle(image_t image, int x, int y, int r, int value, int filled)
{
	if(image == NULL) return -EINVAL;
	if(x-r < 0 || y-r < 0) return -EINVAL;
	if(x+r >= image->width || y+r >= image->height) return -EINVAL;
	
	int x_ = 0, y_ = r;
	int p = 5 - r;
	
	
	 do {
		 if(!filled) {
			 image_set_pixel(image, x+x_, y+y_, value);
			 image_set_pixel(image, x-x_, y+y_, value);
			 image_set_pixel(image, x+x_, y-y_, value);
			 image_set_pixel(image, x-x_, y-y_, value);
			 image_set_pixel(image, x+y_, y+x_, value);
			 image_set_pixel(image, x-y_, y+x_, value);
			 image_set_pixel(image, x+y_, y-x_, value);
			 image_set_pixel(image, x-y_, y-x_, value);
		 } else {
			 image_draw_straight_line(image, x-x_, y+y_, x+x_, value);
			 image_draw_straight_line(image, x-x_, y-y_, x+x_, value);
			 image_draw_straight_line(image, x-y_, y+x_, x+y_, value);
			 image_draw_straight_line(image, x-y_, y-x_, x+y_, value);
		 }
			
		if(p < 0) {
			x_++;
			p += 4*2*x_ + 4*1;
		} else {
			x_++; y_--;
			p += 4*2*x_ + 4*1 - 4*2*y_;
		}
		
	} while(x_<=y_);
	
	return 0;
}

/* Load an image from a pgm file.
 * *target must either be NULL and will then be acquired through image_acquire, or
 * already have its data and size members set.
 * Will use scratch_space as a temporary buffer.
 */
int image_load_pgm(image_t *target, char *filename)
{
	uint32_t preload_size = 512;
	uint8_t *scratch = scratch_space;
	uint8_t *filehead = scratch_space+SECTOR_SIZE;
	FILEINFO fi;
	int res, image_start = -1;
	int width, height, maxval;
	
	if(target == NULL) return -EINVAL;
	if(filename == NULL) return -EINVAL;
	if(sizeof(scratch_space) < SECTOR_SIZE + preload_size) return -EMSGSIZE;
	
	/* Theory of operation: Read the start of the file into the "filehead" buffer
	 * Go through that buffer and operate on it byte-wise:
	 *  + All comments (from # till the end of the line) will be overwritten with spaces
	 *  + All transitions non-whitespace to whitespace will be used as a possible header end
	 *    and we try to parse it with sscanf. If that succeeds and yields the correct number
	 *    of header fields we're in business and the image will start right after that point.
	 */
	
	res = fat_helper_open((unsigned char*)filename, &fi);
	if(res != DFS_OK) {
		switch(res) {
		case DFS_NOTFOUND: return -ENOENT;
		default: return -EIO;
		}
	}
	
	res = DFS_ReadFile(&fi, scratch, filehead, &preload_size, preload_size);
	if(res != DFS_OK && res != DFS_EOF) {
		return -EIO;
	}
	
	if(filehead[0] != 'P' || filehead[1] != '5') {
		return -EILSEQ;
	}
	
	{
		unsigned int i, last_was_space=0, is_comment = 0;
		uint8_t backup;
		
		for(i=0; i<preload_size; i++) {
			if(is_comment) {
				if(filehead[i] == '\r' || filehead[i] == '\n') {
					is_comment = 0;
				} else {
					filehead[i] = ' ';
				}
			} else {
				if(filehead[i] == '#') {
					is_comment = 1;
					filehead[i] = ' ';
				}
			}
			
			switch(filehead[i]) {
			case ' ': case '\t': case '\r': case '\n': case '\v':
				/* Whitespace */
				if(last_was_space) {
					continue;
				}
				last_was_space = 1;
				break;
			default:
				last_was_space = 0;
				continue;
			}
			
			backup = filehead[i]; filehead[i] = 0;
			res = sscanf((const char*)filehead, "P5 %i %i %i", &width, &height, &maxval);
			filehead[i] = backup;
			if(res == 3) {
				/* i is now the white space following the header, the next byte is
				 * the first byte of the image. There might be something more complicated
				 * going on when there was a comment right after the header, but for
				 * now we're going to ignore that case.
				 */
				image_start = i+1;
				break;
			}
		}
	}
	
	if(image_start == -1) {
		/* No PGM start found within the first preload_size bytes */
		return -ENOMSG;
	}
	
	//printf("PGM found: %ix%i at %i colors, starting at offset %i\n", width, height, maxval, image_start);
	if(maxval >= 256) {
		return -ENOTSUP;
	}
	
	DFS_Seek(&fi, image_start, scratch);
	if(fi.pointer != (unsigned int)image_start) {
		return -EIO;
	}
	
	if(*target == NULL) {
		res = image_acquire(target, width, height, width, IMAGE_BPP_8);
		if(res < 0) {
			return res;
		}
	} else {
		if((unsigned)(width*height) > (*target)->size) {
			return -ENOMEM;
		} else {
			(*target)->width = width;
			(*target)->height = height;
			(*target)->rowstride = width;
			(*target)->bits_per_pixel = IMAGE_BPP_8;
		}
	}
	
	uint32_t tmp = (*target)->size;
	res = DFS_ReadFile(&fi, scratch, (*target)->data, &tmp, width*height);
	(*target)->size = tmp;
	if(res != DFS_OK && res != DFS_EOF) {
		return -EIO;
	}
	
	return 0;
}
