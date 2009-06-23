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
#include "eink/eink.h"

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
	struct {
		int x1, y1;
		int x2, y2;
	} damage_region; /* Damaged region since last upload (set all to -1 for n/a) */
	struct {
		int malloced:1; /* data has been acquired through malloc() and needs to be free'd */
	} flags;
	uint8_t *data;
};
typedef struct image *image_t;

struct image_in_image_buffer {
	image_t image; /* In host memory */
	eink_image_buffer_t image_buffer; /* In controller memory */
	struct {
		int x, y; /* Relative to the image_buffer */
		int off_x, off_y; /* Relative to the image */
		int w, h;
	} position; /* Position, offset and crop of image in image buffer */
};
typedef struct image_in_image_buffer *image_in_image_buffer_t;

extern int image_unpack_splash(image_t target, const struct splash_image * const source);
extern int image_create_solid(image_t target, uint8_t color, int width, int height);
extern enum eink_pack_mode image_get_bpp_as_pack_mode(const image_t in);

extern int image_acquire(image_t *target, int width, int height, int rowstride, enum image_bpp bits_per_pixel);
extern int image_release(image_t *target);
extern int image_load_pgm(image_t *target, char *filename);

extern int image_load_image_buffer(image_in_image_buffer_t target, eink_image_buffer_t buffer, image_t source,
		enum eink_rotation_mode rotation_mode);
extern int image_load_image_buffer_centered(image_in_image_buffer_t target, eink_image_buffer_t buffer, image_t source,
		enum eink_rotation_mode rotation_mode);
extern int image_load_image_buffer_position(image_in_image_buffer_t target, eink_image_buffer_t buffer, image_t source,
		enum eink_rotation_mode rotation_mode, int x, int y);
extern int image_load_image_buffer_cropped(image_in_image_buffer_t target, eink_image_buffer_t buffer, image_t source,
		enum eink_rotation_mode rotation_mode, int x, int y, int w, int h);
extern int image_load_image_buffer_offset(image_in_image_buffer_t target, eink_image_buffer_t buffer, image_t source,
		enum eink_rotation_mode rotation_mode, int x, int y, int w, int h, int off_x, int off_y);

extern int image_update_image_buffer(image_in_image_buffer_t target);

extern int image_get_pixel(const struct image * const image, int x, int y);
extern void image_set_pixel(image_t image, int x, int y, uint8_t value);

extern int image_draw_circle(image_t image, int x, int y, int r, int value, int filled);

#endif /* IMAGE_H_ */
