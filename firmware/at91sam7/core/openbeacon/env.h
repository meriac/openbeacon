/***************************************************************
 *
 * OpenBeacon.org - flash routines for persistent environment storage
 *
 * Copyright 2007 Milosch Meriac <meriac@openbeacon.de>
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

#ifndef __ENV_H__
#define __ENV_H__

typedef union
{
  TEnvironment e;
  unsigned int data[AT91C_IFLASH_PAGE_SIZE / sizeof (unsigned int)];
} __attribute__((__packed__)) TEnvironmentBlock;

extern TEnvironmentBlock env;
extern void env_store (void);
extern int env_load (void);
extern void env_init (void);
extern unsigned short env_crc16 (const unsigned char *buffer, int size);
extern void env_flash_to (const void *addr) RAMFUNC;

#if defined(__AT91SAM7X__) || defined(__AT91SAM7SE__)
extern void env_reboot_to_update (void) RAMFUNC;
#endif/*__AT91SAM7X__ || __AT91SAM7SE__*/

#endif/*__ENV_H__*/
