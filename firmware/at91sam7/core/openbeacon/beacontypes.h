/***************************************************************
 *
 * OpenBeacon.org - default types
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

#ifndef __NRF_TYPES__
#define __NRF_TYPES__

#define RAMFUNC __attribute__ ((long_call, section (".ramfunc")))
#define IRQFUNC __attribute__ ((interrupt("IRQ")))
#define FIQFUNC __attribute__ ((interrupt("FIQ")))

//#define true    -1
//#define false   0

typedef unsigned char bool_t;
typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned long u_int32_t;
typedef signed char s_int8_t;
typedef signed short s_int16_t;
typedef int s_int32_t;

typedef struct
{
  u_int8_t size, proto;
} __attribute__((packed)) TBeaconHeader;

#endif/*__NRF_TYPES__*/
