#ifndef SPLASH_H_
#define SPLASH_H_

#define MAX_PART_SIZE (1024*1024)

struct splash_part {
	unsigned int compress_len;
	unsigned int uncompress_len;
	const char *data;
};

struct splash_image {
	unsigned int width;
	unsigned int height;
	enum { SPLASH_BPP_2 = 2, SPLASH_BPP_4 = 4, SPLASH_BPP_8 = 8} bits_per_pixel;
	const struct splash_part * const *splash_parts;
};


#endif /*SPLASH_H_*/
