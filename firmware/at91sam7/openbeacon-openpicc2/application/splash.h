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
	const struct splash_part * const *splash_parts;
};


#endif /*SPLASH_H_*/
