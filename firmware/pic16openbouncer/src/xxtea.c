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

const bank1 u_int32_t tea_key[4] = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};
static bank1 TXxteaEncryption xxtea;

void
xxtea_encode (void)
{
  u_int32_t z, y, sum, tmp, mx;
  u_int8_t p,q,e;

  z = xxtea.block[XXTEA_BLOCK_COUNT - 1];
  sum = 0;

  q = (6+52/XXTEA_BLOCK_COUNT);
  while (q-- > 0)
    {
      sum += 0x9E3779B9UL;
      e = sum >> 2 & 3;
      
      for(p=0; p<XXTEA_BLOCK_COUNT; p++)
      {
        y = xxtea.block[(p+1)&(XXTEA_BLOCK_COUNT-1)];
        tmp = xxtea.block[p];
	z = tmp + ((z>>5^y<<2)+(y>>3^z<<4)^(sum^y)+(tea_key[p&3^e]^z));
        xxtea.block[p] = z;
      }
    }
}
