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
#include <string.h>

#include "utils.h"
#include "eink/eink.h"
#include "lzo/minilzo.h"
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


void image_set_pixel(image_t image, int x, int y, int value)
{
	int mask = ((0xff>>(8-image->bits_per_pixel)))<<( (x*image->bits_per_pixel) % 8);
	int byteindex = y*image->rowstride + (x*image->bits_per_pixel)/8;
	image->data[byteindex] = (image->data[byteindex] & ~mask)
		| ( ((value >> (8-image->bits_per_pixel)) << ((x*image->bits_per_pixel) % 8)) & mask);
}
