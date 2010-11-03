/***************************************************************
 *
 * OpenBeacon.org - FAT file system helper functions
 *
 * Copyright 2009 Henryk Ploetz <henryk@ploetzli.ch>
 *
 ***************************************************************

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/
#ifndef __FAT_HELPER_H__
#define __FAT_HELPER_H__

#include "dosfs.h"

extern int fat_init (void);
extern int fat_helper_open (uint8_t * filename, FILEINFO * fi);
extern int fat_checkimage (uint8_t * filename, size_t * length);

#endif
