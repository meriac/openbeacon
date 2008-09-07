/***************************************************************
 *
 * OpenBeacon.org - XXTEA encryption / decryption
 *                  exported functions
 *
 * Copyright 2006 Milosch Meriac <meriac@openbeacon.de>
 *
/***************************************************************

/*
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

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

extern void protocol_setup_hello (void);
extern void protocol_calc_secret (void);
extern void protocol_setup_response (void);

#endif/*__PROTOCOL_H__*/
