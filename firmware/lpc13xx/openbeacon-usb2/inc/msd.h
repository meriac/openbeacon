/***************************************************************
 *
 * OpenBeacon.org - MSD ROM function code for LPC13xx
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
#ifndef __MSD_H__
#define __MSD_H__

#define DISK_BLOCK_SIZE 512UL

extern void msd_init(void);
extern void msd_read (uint32_t offset, uint8_t dst[], uint32_t length);
extern void msd_write(uint32_t offset, uint8_t src[], uint32_t length);

#endif/*__MSD_H__*/
