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

#define XXTEA_BLOCK_COUNT (BOUNCERPKT_PICKS_LIST_SIZE/4)

typedef union {
  u_int32_t block[XXTEA_BLOCK_COUNT];
  u_int8_t data[BOUNCERPKT_PICKS_LIST_SIZE];
} TXxteaEncryption;

const long tea_key[4] = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};
static bank1 TXxteaEncryption xxtea;
static u_int32_t z, y, sum, tmp, mx;
static u_int8_t q,e;

#define TEA_ROUNDS_COUNT (6+52/XXTEA_BLOCK_COUNT)
#define MX ((z>>5^y<<2)+(y>>3^z<<4)^(sum^y)+(tea_key[p&3^e]^z))
#define DELTA 0x9E3779B9L

void
mx_encode (unsigned char p)
{
  z = tmp + MX;
}

void
xxtea_encode (void)
{
  z = xxtea.block[XXTEA_BLOCK_COUNT - 1];
  sum = 0;

  q = TEA_ROUNDS_COUNT;
  while (q-- > 0)
    {
      sum += DELTA;
      e = sum >> 2 & 3;

      y = xxtea.block[1];
      tmp = xxtea.block[0];
      mx_encode (0);
      xxtea.block[0] = z;

      y = xxtea.block[2];
      tmp = xxtea.block[1];
      mx_encode (1);
      xxtea.block[1] = z;

      y = xxtea.block[3];
      tmp = xxtea.block[2];
      mx_encode (2);
      xxtea.block[2] = z;

      y = xxtea.block[4];
      tmp = xxtea.block[3];
      mx_encode (3);
      xxtea.block[3] = z;

      y = xxtea.block[5];
      tmp = xxtea.block[4];
      mx_encode (4);
      xxtea.block[4] = z;

      y = xxtea.block[6];
      tmp = xxtea.block[5];
      mx_encode (5);
      xxtea.block[5] = z;

      y = xxtea.block[7];
      tmp = xxtea.block[6];
      mx_encode (6);
      xxtea.block[6] = z;

      y = xxtea.block[0];
      tmp = xxtea.block[7];
      mx_encode (7);
      xxtea.block[7] = z;
    }
}
