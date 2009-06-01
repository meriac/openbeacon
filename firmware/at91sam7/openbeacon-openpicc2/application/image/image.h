/*
 * image.h
 *
 *  Created on: 01.06.2009
 *      Author: henryk
 */

#ifndef IMAGE_H_
#define IMAGE_H_

#include <stdint.h>
#include "image/splash.h"

enum image_bpp {
	IMAGE_BPP_2 = 2,
	IMAGE_BPP_4 = 4,
	IMAGE_BPP_8 = 8,
};

struct image {
	int width;         /* In pixels */
	int height;        /* In pixels */
	int rowstride;     /* In bytes */
	enum image_bpp bits_per_pixel;
	unsigned int size; /* In bytes */
	uint8_t *data;
};
typedef struct image *image_t;

extern int image_unpack_splash(image_t target, const struct splash_image * const source);
extern int image_create_solid(image_t target, uint8_t color, int width, int height);

#endif /* IMAGE_H_ */
