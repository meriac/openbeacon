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
 * ripped into pieces - optimized for 8 bit PIC microcontroller
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

#define FLASH_SALT_COUNT 12
#define LAST_RND_COUNT 16
#define XXTEA_BLOCK_COUNT (BOUNCERPKT_PICKS_LIST_SIZE/4)

eeprom u_int8_t ee_counter_init = 0xFF;
eeprom u_int32_t ee_counter;
eeprom u_int8_t ee_last_rnd[LAST_RND_COUNT];
eeprom u_int8_t ee_salt[LAST_RND_COUNT];

static volatile const u_int32_t XXTeaKey[4] =
  { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };

static volatile const def_ee_counter = 0xFFFFFFFF;
static volatile const u_int8_t flash_salt[FLASH_SALT_COUNT] = {
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF
};

typedef struct
{
  u_int8_t last_rnd[LAST_RND_COUNT];
  u_int32_t counter;
  u_int8_t salt[FLASH_SALT_COUNT];
} TSaltGeneration;

typedef union
{
  u_int32_t block[XXTEA_BLOCK_COUNT];
  u_int8_t data[BOUNCERPKT_PICKS_LIST_SIZE];
  TSaltGeneration rnd;
} TXXTEAEncryption;
  
static bank1 TXXTEAEncryption xxtea;

static void
xxtea_memcpy(void* dest,const void *src,u_int8_t size)
{
}

static void
xxtea_init_salt (void)
{    
  u_int32_t temp;
  
  if(ee_counter_init)
  {
    ee_counter=def_ee_counter;
    ee_counter_init=0;
  }
  
  temp=ee_counter;
  ee_counter=temp+1;
    
  xxtea_memcpy(&xxtea.rnd.salt,&flash_salt,sizeof(xxtea.rnd.salt));
  xxtea_memcpy(&xxtea.rnd.counter,&temp,sizeof(xxtea.rnd.counter));  
}

static void
xxtea_shuffle_byte_order (void)
{
  u_int8_t t, i, *p;

  p = xxtea.data;
  i = BOUNCERPKT_PICKS_LIST_SIZE;
  while (i--)
    {
      t = *p;
      *p = p[3];
      p[3] = t;
      p++;

      t = *p;
      *p = p[1];
      p[1] = t;

      p += 3;
    }
}

void
xxtea_encode (void)
{
  u_int32_t z, y, sum, mx;
  u_int8_t p, q, e;

  /* adjust byte order to match network byte order for transmission */
  xxtea_shuffle_byte_order ();

  /* prepare first XXTEA round */
  z = xxtea.block[XXTEA_BLOCK_COUNT - 1];
  sum = 0;

  q = (6 + 52 / XXTEA_BLOCK_COUNT);
  while (q--)
    {
      sum += 0x9E3779B9UL;
      e = (sum >> 2) & 3;

      for (p = 0; p < XXTEA_BLOCK_COUNT; p++)
	{
	  y = xxtea.block[(p + 1) & (XXTEA_BLOCK_COUNT - 1)];
	  z =
	    xxtea.block[p] + ((z >> 5 ^ y << 2) +
			      (y >> 3 ^ z << 4) ^ (sum ^ y) +
			      (XXTeaKey[p & 3 ^ e] ^ z));
	  xxtea.block[p] = z;
	}
    }

  /* adjust byte order to match network byte order for transmission */
  xxtea_shuffle_byte_order ();
}
