#ifndef __FAT_HELPER_H__
#define __FAT_HELPER_H__

#include "eink/eink.h"

extern int fat_init(void);
extern int fat_load_eink_image_buffer(char *filename, size_t *length, eink_image_buffer_t img_buf);
extern int fat_load_data_buffer(char *filename, size_t *length, uint8_t *buffer);

#endif
