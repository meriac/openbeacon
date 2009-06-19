#ifndef __FAT_HELPER_H__
#define __FAT_HELPER_H__

#include "eink/eink.h"

extern int fat_init(void);
extern int fat_loadimage( uint8_t *filename, size_t length, eink_image_buffer_t img_buf );

#endif
