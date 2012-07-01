/***************************************************************
 *
 * OpenBeacon.org - XXTEA encryption / decryption
 *                  exported functions
 *
 * Copyright 2006 Milosch Meriac <meriac@openbeacon.de>
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

#ifndef __XXTEA_H__
#define __XXTEA_H__

#ifdef  CONFIG_TEA_ENABLEENCODE
extern void xxtea_encode (void);
#endif /*CONFIG_ENABLEENCODE */

#ifdef  CONFIG_TEA_ENABLEDECODE
extern void xxtea_decode (void);
#endif /*CONFIG_ENABLEDECODE */

#endif/*__XXTEA_H__*/
