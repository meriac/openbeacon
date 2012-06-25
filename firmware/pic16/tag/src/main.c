/***************************************************************
 *
 * OpenBeacon.org - main entry, CRC, behaviour
 *
 * Copyright 2006-2012 Milosch Meriac <meriac@openbeacon.de>
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

#define OPENBEACON_ENABLEENCODE

#include <htc.h>
#include <stdlib.h>
#include "config.h"
#include "timer.h"
#include "nRF_CMD.h"
#include "nRF_HW.h"
#include "openbeacon.h"

__CONFIG (0x03d4);
__EEPROM_DATA (0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

volatile const u_int32_t oid = 0xFFFFFFFF, seed = 0xFFFFFFFF;

static const long xxtea_key[4] =
	{ 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
static u_int32_t seq = 0;
static u_int16_t code_block;

typedef struct
{
	u_int8_t size1, opcode1, rf_setup;
	u_int8_t size2, opcode2;
	TBeaconEnvelope env;
	u_int8_t eot;
} TMacroBeacon;

static TMacroBeacon g_MacroBeacon = {
	0x02,																		// Size
	NRF_REG_RF_SETUP | WRITE_REG,												// Setup RF Options
	RFB_RFOPTIONS,

	sizeof (TBeaconEnvelope) + 1,												// Size
	WR_TX_PLOAD																	// Transmit Packet Opcode
};

static unsigned long
htonl (unsigned long src)
{
	unsigned long res;

	((unsigned char *) &res)[0] = ((unsigned char *) &src)[3];
	((unsigned char *) &res)[1] = ((unsigned char *) &src)[2];
	((unsigned char *) &res)[2] = ((unsigned char *) &src)[1];
	((unsigned char *) &res)[3] = ((unsigned char *) &src)[0];

	return res;
}

static unsigned short
htons (unsigned short src)
{
	unsigned short res;

	((unsigned char *) &res)[0] = ((unsigned char *) &src)[1];
	((unsigned char *) &res)[1] = ((unsigned char *) &src)[0];

	return res;
}

#define SHUFFLE(a,b) 	tmp=g_MacroBeacon.env.datab[a];\
			g_MacroBeacon.env.datab[a]=g_MacroBeacon.env.datab[b];\
			g_MacroBeacon.env.datab[b]=tmp;

static void
shuffle_tx_byteorder (void)
{
	unsigned char tmp;

	SHUFFLE (0 + 0, 3 + 0);
	SHUFFLE (1 + 0, 2 + 0);
	SHUFFLE (0 + 4, 3 + 4);
	SHUFFLE (1 + 4, 2 + 4);
	SHUFFLE (0 + 8, 3 + 8);
	SHUFFLE (1 + 8, 2 + 8);
	SHUFFLE (0 + 12, 3 + 12);
	SHUFFLE (1 + 12, 2 + 12);
}

static unsigned short
crc16 (const unsigned char *buffer, unsigned char size)
{
	unsigned short crc = 0xFFFF;
	if (buffer)
	{
		while (size--)
		{
			crc = (crc >> 8) | (crc << 8);
			crc ^= *buffer++;
			crc ^= ((unsigned char) crc) >> 4;
			crc ^= crc << 12;
			crc ^= (crc & 0xFF) << 5;
		}
	}
	return crc;
}

static void
store_incremented_codeblock (void)
{
	if (code_block < 0xFFFF)
	{
		code_block++;
		EEPROM_WRITE (0, (u_int8_t) (code_block));
		sleep_jiffies (JIFFIES_PER_MS (10));
		EEPROM_WRITE (1, (u_int8_t) (code_block >> 8));
		sleep_jiffies (JIFFIES_PER_MS (10));
	}
}

static void
xxtea_encode (void)
{
	u_int32_t z, y, sum;
	u_int8_t p, q, e;

	/* prepare first XXTEA round */
	z = g_MacroBeacon.env.data[TEA_ENCRYPTION_BLOCK_COUNT - 1];
	sum = 0;

	/* setup rounds counter */
	q = (6 + 52 / TEA_ENCRYPTION_BLOCK_COUNT);

	/* start encryption */
	while (q--)
	{
		sum += 0x9E3779B9UL;
		e = (sum >> 2) & 3;

		for (p = 0; p < TEA_ENCRYPTION_BLOCK_COUNT; p++)
		{
			y = g_MacroBeacon.
				env.data[(p + 1) & (TEA_ENCRYPTION_BLOCK_COUNT - 1)];
			z = g_MacroBeacon.env.data[p] + ((z >> 5 ^ y << 2) +
											 (y >> 3 ^ z << 4) ^ (sum ^ y) +
											 (xxtea_key[p & 3 ^ e] ^ z));
			g_MacroBeacon.env.data[p] = z;
		}
	}
}

void
main (void)
{
	u_int8_t i;
	u_int16_t crc;

	/* configure CPU peripherals */
	OPTION_REG = CONFIG_CPU_OPTION;
	TRISA = CONFIG_CPU_TRISA;
	TRISC = CONFIG_CPU_TRISC;
	WPUA = CONFIG_CPU_WPUA;
	ANSEL = CONFIG_CPU_ANSEL;
	CMCON0 = CONFIG_CPU_CMCON0;
	OSCCON = CONFIG_CPU_OSCCON;
#ifndef CONFIG_HIRES_LOCATION
	CONFIG_PIN_TX_POWER = NRF_TX_POWER_HIGH;
#endif /*CONFIG_HIRES_LOCATION */

	timer_init ();

	for (i = 0; i <= 40; i++)
	{
		CONFIG_PIN_LED = (i & 1) ? 1 : 0;
		sleep_jiffies (JIFFIES_PER_MS (100));
	}

	nRFCMD_Init ();

	IOCA = IOCA | (1 << 0);

	// increment code block after power cycle
	((unsigned char *) &code_block)[0] = EEPROM_READ (0);
	((unsigned char *) &code_block)[1] = EEPROM_READ (1);
	store_incremented_codeblock ();

	seq = code_block ^ oid ^ seed;
	srand (crc16 ((unsigned char *) &seq, sizeof (seq)));

	// increment code blocks to make sure that seq is higher or equal after battery
	// change
	seq = ((u_int32_t) (code_block - 1)) << 16;

	i = 0;
	if (code_block != 0xFFFF)
		while (1)
		{
			g_MacroBeacon.rf_setup = NRF_RFOPTIONS | ((i & 3) << 1);
			g_MacroBeacon.env.pkt.hdr.proto = RFBPROTO_BEACONTRACKER;
			g_MacroBeacon.env.pkt.hdr.flags =
				CONFIG_PIN_SENSOR ? 0 : RFBFLAGS_SENSOR;
			g_MacroBeacon.env.pkt.hdr.oid = htons ((u_int16_t) oid);
			g_MacroBeacon.env.pkt.oid_last_seen = 0;
			g_MacroBeacon.env.pkt.powerup_count =
				htons ((u_int16_t) code_block);
			g_MacroBeacon.env.pkt.strength = i;
			g_MacroBeacon.env.pkt.seq = htonl (seq);
			g_MacroBeacon.env.pkt.reserved = 0;
			crc = crc16 (g_MacroBeacon.env.datab,
						 sizeof (g_MacroBeacon.env.pkt) -
						 sizeof (g_MacroBeacon.env.pkt.crc));
			g_MacroBeacon.env.pkt.crc = htons (crc);

			// update code_block so on next power up
			// the seq will be higher or equal
			crc = seq >> 16;
			if (crc == 0xFFFF)
				break;
			if (crc == code_block)
				store_incremented_codeblock ();

			// encrypt my data
			shuffle_tx_byteorder ();
			xxtea_encode ();
			shuffle_tx_byteorder ();

			// reset touch sensor pin
			TRISA = CONFIG_CPU_TRISA & ~0x02;
			CONFIG_PIN_SENSOR = 0;
			sleep_jiffies (JIFFIES_PER_MS (32 + (rand () % 80)));
			CONFIG_PIN_SENSOR = 1;
			TRISA = CONFIG_CPU_TRISA;

#ifdef CONFIG_HIRES_LOCATION
			if ((i & 4) == 0)
				CONFIG_PIN_TX_POWER = NRF_TX_POWER_LOW;
#endif	/* CONFIG_HIRES_LOCATION */

			// send it away
			nRFCMD_Macro ((unsigned char *) &g_MacroBeacon);

			if (!i && ((((unsigned char) seq) & 0xF) == 0))
				CONFIG_PIN_LED = 1;
			nRFCMD_Execute ();
			CONFIG_PIN_LED = 0;
#ifdef CONFIG_HIRES_LOCATION
			CONFIG_PIN_TX_POWER = NRF_TX_POWER_HIGH;
#endif	/* CONFIG_HIRES_LOCATION */
			if (++i >= CONFIG_MAX_POWER_LEVELS)
			{
				i = 0;
				seq++;
			}
		}

	// rest in peace
	// when seq counter is exhausted
	while (1)
	{
		sleep_jiffies (JIFFIES_PER_MS (95));
		CONFIG_PIN_LED = 1;
		sleep_jiffies (JIFFIES_PER_MS (5));
		CONFIG_PIN_LED = 0;
	}
}
