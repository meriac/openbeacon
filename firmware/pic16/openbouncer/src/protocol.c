/***************************************************************
 *
 * OpenBeacon.org -  OpenBouncer cryptographic door lock
 *                   protocol implementation
 *
 * Copyright 2006 Milosch Meriac <meriac@openbeacon.de>
 *
 * XXTEA encryption as seen in "correction to XTEA"
 * by David J. Wheeler and Roger M. Needham.
 * Computer Laboratory - Cambridge University in 1998
 * but optimized for microcontroller usage and low memory
 * and code footprint
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
#include "protocol.h"
#include "main.h"
#include "timer.h"

#define FLASHSALT_BLOCKS 7
#define LASTRND_BLOCKS 4

typedef struct
{
  u_int32_t counter;
  u_int32_t flashsalt[FLASHSALT_BLOCKS];
} TSaltGeneration;

typedef struct
{
  u_int32_t salt_a;
  u_int32_t salt_b;
} TEntropy;

typedef struct
{
  u_int32_t salt_a;
  u_int32_t salt_b;
  u_int32_t challenge[2];
  u_int32_t lock_id;
  u_int32_t zero[3];
} TResponse;

typedef union
{
  u_int32_t block[BOUNCERPKT_XXTEA_BLOCK_COUNT];
  u_int8_t data_b[BOUNCERPKT_XXTEA_BLOCK_COUNT * 4];
  TSaltGeneration rnd;
  TEntropy entropy;
  TResponse response;
} TXXTEAEncryption;

static eeprom u_int32_t ee_counter = 0xFFFFFFFF;

volatile const def_ee_counter = 0xFFFFFFFF;
volatile const u_int32_t xxtea_key[4] =
  { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
volatile const u_int32_t flashsalt[FLASHSALT_BLOCKS] = {
  0xFFFFFFFF, 0xFFFFFFFF,
  0xFFFFFFFF, 0xFFFFFFFF,
  0xFFFFFFFF, 0xFFFFFFFF,
  0xFFFFFFFF
};

static bank1 TXXTEAEncryption xxtea;
static u_int32_t xxtea_salt_b;
static u_int8_t xxtea_retries = 0;

#define ntohl htonl
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

static void
xxtea_encode (void)
{
  u_int32_t z, y, sum, mx;
  u_int8_t p, q, e;

  /* prepare first XXTEA round */
  z = xxtea.block[BOUNCERPKT_XXTEA_BLOCK_COUNT - 1];
  sum = 0;

  /* setup rounds counter */
  q = (6 + 52 / BOUNCERPKT_XXTEA_BLOCK_COUNT);

  /* start encryption */
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

u_int8_t
protocol_setup_hello (void)
{
  u_int8_t i;

  if (ee_counter == 0xFFFF)
    return 0;

  xxtea.rnd.counter = ee_counter;
  ee_counter = xxtea.rnd.counter + 1;

  for (i = 0; i < FLASHSALT_BLOCKS; i++)
    xxtea.rnd.flashsalt[i] = flashsalt[i];

  /* XXTEA over (last_rnd || counter || flash_salt) to calculate
     salt_a, salt_b and to update lastrnd */
  xxtea_encode ();

  g_MacroBeacon.cmd.hdr.version = BOUNCERPKT_VERSION;
  g_MacroBeacon.cmd.hdr.command = BOUNCERPKT_CMD_HELLO;
  g_MacroBeacon.cmd.hdr.value = xxtea_retries++;
  g_MacroBeacon.cmd.hdr.flags = 0;
  g_MacroBeacon.cmd.hello.salt_a = htonl (xxtea.entropy.salt_a);
  xxtea_salt_b = htonl (xxtea.entropy.salt_b);
  g_MacroBeacon.cmd.hello.reserved[0] = ee_counter;
  g_MacroBeacon.cmd.hello.reserved[1] = 0;

  return 1;
}

u_int8_t
protocol_calc_secret (void)
{
  u_int8_t i;

  /* salt_a and salt_b set in xxtea during previous setup_hello */
  xxtea.response.challenge[0] =
    ntohl (g_MacroBeacon.cmd.challenge_setup.challenge[0]);
  xxtea.response.challenge[1] =
    ntohl (g_MacroBeacon.cmd.challenge_setup.challenge[1]);
  xxtea.response.lock_id = ntohl (g_MacroBeacon.cmd.challenge_setup.src_mac);
  xxtea.response.zero[0] = 0;
  xxtea.response.zero[1] = 0;
  xxtea.response.zero[2] = 0;

  /* calculate response over (salt_a || salt_b || challenge || lock_id || 0 ... ) */
  xxtea_encode ();

  /* fix endianess so picks byte array is architecture independent */
  for (i = 0; i < BOUNCERPKT_XXTEA_BLOCK_COUNT; i++)
    xxtea.block[i] = htonl (xxtea.block[i]);

  return 1;
}

u_int8_t
protocol_setup_response (void)
{
  u_int8_t i, t;

  g_MacroBeacon.cmd.hdr.version = BOUNCERPKT_VERSION;
  g_MacroBeacon.cmd.hdr.command = BOUNCERPKT_CMD_RESPONSE;
  g_MacroBeacon.cmd.hdr.value = BOUNCERPKT_PICKS_LIST_SIZE;
  g_MacroBeacon.cmd.hdr.flags = 0;
  g_MacroBeacon.cmd.response.salt_b = xxtea_salt_b;

  for (i = 0; i < BOUNCERPKT_PICKS_COUNT; i++)
    {
      t = g_MacroBeacon.cmd.challenge.picks[i];
      g_MacroBeacon.cmd.response.picks[i] =
	(t < sizeof (xxtea.data_b)) ? xxtea.data_b[t] : 0;
    }

  return 1;
}
