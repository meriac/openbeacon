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
#ifndef __PERSISTENT_H__
#define __PERSISTENT_H__

#ifdef ENABLE_PERSISTENT_INITIALIZATION
#define PERSISTENT __attribute__ ((section (".persistent")))
extern void persistent_update(void);
extern void persistent_init (void);
#endif /*ENABLE_PERSISTENT_INITIALIZATION*/

#endif/*__PERISTENT_H__*/
