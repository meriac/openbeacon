/***************************************************************
 *
 * OpenBeacon.org - FLASH storage support
 *
 * Copyright 2010 Milosch Meriac <meriac@openbeacon.de>
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

#ifndef __STORAGE_H__
#define __STORAGE_H__

#include "msd.h"

#define LOGFILE_STORAGE_SIZE (4*1024*1024)

extern void storage_init (uint8_t usb_enabled, uint16_t device_id);
extern void storage_read (uint32_t offset, uint32_t length, void *dst);
extern void storage_write (uint32_t offset, uint32_t length, const void *src);
extern uint32_t storage_items (void);
extern void storage_erase (void);
extern void storage_status (void);

#endif/*__STORAGE_H__*/
