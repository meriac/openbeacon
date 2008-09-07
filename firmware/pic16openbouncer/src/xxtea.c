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
#include "openbouncer.h"
#include "xxtea.h"
#include "main.h"

#define FLASHSALT_BLOCKS 3
#define LASTRND_BLOCKS 4

typedef struct
{
  u_int32_t lastrnd[LASTRND_BLOCKS];
  u_int32_t counter;
  u_int32_t flashsalt[FLASHSALT_BLOCKS];
} TSaltGeneration;

typedef struct
{
  u_int32_t salt_a;
  u_int32_t salt_b;
  u_int32_t reserved[2];
  u_int32_t new_flashsalt[FLASHSALT_BLOCKS];
} TResponsePool;

typedef union
{
  u_int32_t block[BOUNCERPKT_XXTEA_BLOCK_COUNT];
  u_int8_t data[BOUNCERPKT_XXTEA_BLOCK_COUNT * 4];
  TSaltGeneration rnd;
  TResponsePool res;
} TXXTEAEncryption;

static eeprom u_int32_t ee_counter = 0xFFFFFFFF;
eeprom u_int32_t ee_lastrnd[LASTRND_BLOCKS] =
  { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };

volatile const def_ee_counter = 0xFFFFFFFF;
volatile const u_int32_t xxtea_key[4] =
  { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
volatile const u_int32_t flashsalt[FLASHSALT_BLOCKS] =
  { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };

static bank1 TXXTEAEncryption xxtea;
static u_int32_t xxtea_salt_b;
static u_int8_t xxtea_retries = 0;

static u_int32_t
htonl (u_int32_t src)
{
  u_int32_t res;

  ((u_int8_t *) & res)[0] = ((u_int8_t *) & src)[3];
  ((u_int8_t *) & res)[1] = ((u_int8_t *) & src)[2];
  ((u_int8_t *) & res)[2] = ((u_int8_t *) & src)[1];
  ((u_int8_t *) & res)[3] = ((u_int8_t *) & src)[0];

  return res;
}

static u_int16_t
htons (u_int16_t src)
{
  u_int16_t res;

  ((u_int8_t *) & res)[0] = ((u_int8_t *) & src)[1];
  ((u_int8_t *) & res)[1] = ((u_int8_t *) & src)[0];

  return res;
}

static void
xxtea_encode (void)
{
  u_int32_t z, y, sum, mx;
  u_int8_t p, q, e;

  /* prepare first XXTEA round */
  z = xxtea.block[BOUNCERPKT_XXTEA_BLOCK_COUNT - 1];
  sum = 0;

  q = (6 + 52 / BOUNCERPKT_XXTEA_BLOCK_COUNT);
  while (q--)
    {
      sum += 0x9E3779B9UL;
      e = (sum >> 2) & 3;

      for (p = 0; p < BOUNCERPKT_XXTEA_BLOCK_COUNT; p++)
	{
	  y = xxtea.block[(p + 1) & (BOUNCERPKT_XXTEA_BLOCK_COUNT - 1)];
	  z =
	    xxtea.block[p] + ((z >> 5 ^ y << 2) +
			      (y >> 3 ^ z << 4) ^ (sum ^ y) +
			      (xxtea_key[p & 3 ^ e] ^ z));
	  xxtea.block[p] = z;
	}
    }
}

void
xxtea_setup_hello (void)
{
  u_int8_t i;

  xxtea.rnd.counter = ee_counter;
  ee_counter = xxtea.rnd.counter + 1;

  for (i = 0; i < FLASHSALT_BLOCKS; i++)
    xxtea.rnd.flashsalt[i] = flashsalt[i];

  for (i = 0; i < LASTRND_BLOCKS; i++)
    xxtea.rnd.lastrnd[i] = ee_lastrnd[i];

  /* XXTEA over (last_rnd || counter || flash_salt) to calculate
     salt_a, salt_b and to update lastrnd */
  xxtea_encode ();

  for (i = 0; i < LASTRND_BLOCKS; i++)
    ee_lastrnd[i] =
      xxtea.rnd.lastrnd[i + (BOUNCERPKT_XXTEA_BLOCK_COUNT - LASTRND_BLOCKS)];

  g_MacroBeacon.cmd.hdr.version = BOUNCERPKT_VERSION;
  g_MacroBeacon.cmd.hdr.command = BOUNCERPKT_CMD_HELLO;
  g_MacroBeacon.cmd.hdr.value = xxtea_retries++;;
  g_MacroBeacon.cmd.hdr.flags = 0;
  g_MacroBeacon.cmd.hello.salt_a = htonl(xxtea.res.salt_a);
  g_MacroBeacon.cmd.hello.reserved[0] = 0;
  g_MacroBeacon.cmd.hello.reserved[1] = 0;
}

void
xxtea_setup_response_from_challenge (void)
{

}
