/***************************************************************
 *
 * OpenBeacon.org - XXTEA based block encryption
 *
 * based on "correction to XTEA" by David J. Wheeler and
 * Roger M. Needham.
 * Computer Laboratory - Cambridge University in 1998
 *
 * Copyright 2006 Milosch Meriac <meriac@openbeacon.de>
 *
 * ripped into pieces - optimized for the special case
 * where 16 bytes are regarded for encryption and decryption
 * to increase speed and to decrease memory consumption
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

#include <beacontypes.h>
#include <board.h>
#include "xxtea.h"
#include "proto.h"

/*
 * Default TEA encryption key of the tag - MUST CHANGE !
 */
const long tea_key[4] = { 0x00112233, 0x44556677, 0x8899AABB, 0xCCDDEEFF };

#define TEA_ROUNDS_COUNT (6+52/4)
#define MX ((((z>>5)^(y<<2))+((y>>3)^(z<<4)))^((sum^y)+(tea_key[(p&3)^e]^z)))
#define DELTA 0x9E3779B9L

unsigned long z, y, sum, tmp, mx;
unsigned char e;

void RAMFUNC
mx_update (unsigned char p)
{
	mx = MX;
}

#ifdef  CONFIG_TEA_ENABLEENCODE
static inline void RAMFUNC
mx_encode (unsigned char p)
{
	mx_update (p);
	z = tmp + mx;
}

void RAMFUNC
xxtea_encode (void)
{
	int q;

	z = g_Beacon.block[3];
	sum = 0;

	q = TEA_ROUNDS_COUNT;
	while (q-- > 0)
	{
		sum += DELTA;
		e = sum >> 2 & 3;

		y = g_Beacon.block[1];
		tmp = g_Beacon.block[0];
		mx_encode (0);
		g_Beacon.block[0] = z;

		y = g_Beacon.block[2];
		tmp = g_Beacon.block[1];
		mx_encode (1);
		g_Beacon.block[1] = z;

		y = g_Beacon.block[3];
		tmp = g_Beacon.block[2];
		mx_encode (2);
		g_Beacon.block[2] = z;

		y = g_Beacon.block[0];
		tmp = g_Beacon.block[3];
		mx_encode (3);
		g_Beacon.block[3] = z;
	}
}
#endif /*CONFIG_TEA_ENABLEENCODE */

#ifdef  CONFIG_TEA_ENABLEDECODE
static inline void RAMFUNC
mx_decode (unsigned char p)
{
	mx_update (p);
	y = tmp - mx;
}

void RAMFUNC
xxtea_decode (void)
{
	y = g_Beacon.block[0];
	sum = DELTA * TEA_ROUNDS_COUNT;

	while (sum != 0)
	{
		e = sum >> 2 & 3;

		z = g_Beacon.block[2];
		tmp = g_Beacon.block[3];
		mx_decode (3);
		g_Beacon.block[3] = y;

		z = g_Beacon.block[1];
		tmp = g_Beacon.block[2];
		mx_decode (2);
		g_Beacon.block[2] = y;

		z = g_Beacon.block[0];
		tmp = g_Beacon.block[1];
		mx_decode (1);
		g_Beacon.block[1] = y;

		z = g_Beacon.block[3];
		tmp = g_Beacon.block[0];
		mx_decode (0);
		g_Beacon.block[0] = y;

		sum -= DELTA;
	}
}
#endif /*CONFIG_TEA_ENABLEDECODE */
