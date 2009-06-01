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
	return 0;
}
