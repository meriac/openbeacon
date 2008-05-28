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

#include "config.h"
#include "xxtea.h"
#include "main.h"

#define CONFIG_TEA_ENCRYPTION_BLOCK_COUNT 4

const long tea_key[4] = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};
unsigned long z, y, sum, tmp, mx;
unsigned char e;

#define TEA_ROUNDS_COUNT (6+52/CONFIG_TEA_ENCRYPTION_BLOCK_COUNT)
#define MX ((z>>5^y<<2)+(y>>3^z<<4)^(sum^y)+(tea_key[p&3^e]^z))
#define DELTA 0x9E3779B9L

void
mx_update (unsigned char p)
{
  mx = MX;
}

#ifdef  CONFIG_TEA_ENABLEENCODE
void
mx_encode (unsigned char p)
{
  mx_update (p);
  z = tmp + mx;
}

void
xxtea_encode (void)
{
  unsigned char q;

  z = g_MacroBeacon.env.data[CONFIG_TEA_ENCRYPTION_BLOCK_COUNT - 1];
  sum = 0;

  q = TEA_ROUNDS_COUNT;
  while (q-- > 0)
    {
      sum += DELTA;
      e = sum >> 2 & 3;

      y = g_MacroBeacon.env.data[1];
      tmp = g_MacroBeacon.env.data[0];
      mx_encode (0);
      g_MacroBeacon.env.data[0] = z;

      y = g_MacroBeacon.env.data[2];
      tmp = g_MacroBeacon.env.data[1];
      mx_encode (1);
      g_MacroBeacon.env.data[1] = z;

      y = g_MacroBeacon.env.data[3];
      tmp = g_MacroBeacon.env.data[2];
      mx_encode (2);
      g_MacroBeacon.env.data[2] = z;

      y = g_MacroBeacon.env.data[0];
      tmp = g_MacroBeacon.env.data[3];
      mx_encode (3);
      g_MacroBeacon.env.data[3] = z;
    }
}
#endif /*CONFIG_TEA_ENABLEENCODE */

#ifdef  CONFIG_TEA_ENABLEDECODE
void
mx_decode (unsigned char p)
{
  mx_update (p);
  y = tmp - mx;
}

void
xxtea_decode (void)
{
  unsigned char q;

  y = g_MacroBeacon.env.data[0];
  sum = DELTA * TEA_ROUNDS_COUNT;

  while (sum != 0)
    {
      e = sum >> 2 & 3;

      z = g_MacroBeacon.env.data[2];
      tmp = g_MacroBeacon.env.data[3];
      mx_decode (3);
      g_MacroBeacon.env.data[3] = y;

      z = g_MacroBeacon.env.data[1];
      tmp = g_MacroBeacon.env.data[2];
      mx_decode (2);
      g_MacroBeacon.env.data[2] = y;

      z = g_MacroBeacon.env.data[0];
      tmp = g_MacroBeacon.env.data[1];
      mx_decode (1);
      g_MacroBeacon.env.data[1] = y;

      z = g_MacroBeacon.env.data[3];
      tmp = g_MacroBeacon.env.data[0];
      mx_decode (0);
      g_MacroBeacon.env.data[0] = y;

      sum -= DELTA;
    }
}
#endif /*CONFIG_TEA_ENABLEDECODE */
