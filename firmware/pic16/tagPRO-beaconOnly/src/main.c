/***************************************************************
 *
 * OpenBeacon.org 2.4GHz proximity tag - main entry, CRC, behaviour
 *
 * Copyright (C) 2012 Milosch Meriac <meriac@openbeacon.de>
 * - Ported to PIC16LF1825
 *
 * Copyright (C) 2011 Milosch Meriac <meriac@openbeacon.de>
 * - Unified Proximity/Tracking-Mode into one Firmware
 * - Increased Tracking/Proximity rate to 12 packets per second
 * - Implemented Tag-ID assignment/management via RF interface to
 *   simplify production process
 *
 * Copyright (C) 2008 Istituto per l'Interscambio Scientifico I.S.I.
 * - extended by Ciro Cattuto <ciro.cattuto@gmail.com> by support for
 *   SocioPatterns.org platform
 *
 * Copyright (C) 2006 Milosch Meriac <meriac@openbeacon.de>
 * - Optimized en-/decryption routines, CRC's, EEPROM handling
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

#include <htc.h>
#include <stdlib.h>
#include <stdint.h>
#include "config.h"
#include "openbeacon.h"
#include "timer.h"
#include "nRF_CMD.h"
#include "nRF_HW.h"

#define XXTEA_ROUNDS (6UL + 52UL / XXTEA_BLOCK_COUNT)
#define DELTA 0x9E3779B9UL

/* EEPROM location definitions */
#define EEPROM_CODE_BLOCK_H 0
#define EEPROM_CODE_BLOCK_L 1
#define EEPROM_OID_H 2
#define EEPROM_OID_L 3

/*    configuration word 1
      32 1098 7654 3210
      00 1001 1100 1100 = 0x0DCC */
__CONFIG (0x09CC);

/*    configuration word 2
      32 1098 7654 3210
      01 0110 0001 0000 = 0x1610 */
__CONFIG (0x1610);

__EEPROM_DATA (0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);

// key is set in create_counted_firmware.php while patching firmware hex file
volatile const uint32_t oid = 0xFFFFFFFF;
volatile const uint32_t seed = 0xFFFFFFFF;
const uint32_t xxtea_key[4] =
{
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF
};

static uint32_t seq = 0;
static uint16_t oid_ram = 0;
static uint16_t code_block;
static uint8_t global_flags = 0;

static uint32_t z, y, sum;
static uint8_t e;
static int8_t p;

static TBeaconEnvelope pkt;

#define ntohs htons
static uint16_t
htons (uint16_t src)
{
	uint16_t res;

	((unsigned char *) &res)[0] = ((unsigned char *) &src)[1];
	((unsigned char *) &res)[1] = ((unsigned char *) &src)[0];

	return res;
}

#define ntohl htonl
static uint32_t
htonl (uint32_t src)
{
	uint32_t res;

	((uint8_t *) & res)[0] = ((uint8_t *) & src)[3];
	((uint8_t *) & res)[1] = ((uint8_t *) & src)[2];
	((uint8_t *) & res)[2] = ((uint8_t *) & src)[1];
	((uint8_t *) & res)[3] = ((uint8_t *) & src)[0];

	return res;
}

static void
store_incremented_codeblock (void)
{
	code_block++;
	eeprom_write (EEPROM_CODE_BLOCK_L, (unsigned char) (code_block));
	eeprom_write (EEPROM_CODE_BLOCK_H, (unsigned char) (code_block >> 8));
}

static uint16_t
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
protocol_fix_endianess (void)
{
	for (p = 0; p < XXTEA_BLOCK_COUNT; p++)
		pkt.block[p] = htonl (pkt.block[p]);
}

static uint32_t
protocol_mx (void)
{
	return ((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4) ^ (sum ^ y) +
			(xxtea_key[p & 3 ^ e] ^ z));
}

static void
protocol_encode (void)
{
	unsigned char i;

	protocol_fix_endianess ();

	z = pkt.block[XXTEA_BLOCK_COUNT - 1];
	sum = 0;

	for (i = XXTEA_ROUNDS; i > 0; i--)
	{
		sum += DELTA;
		e = sum >> 2 & 3;

		for (p = 0; p < XXTEA_BLOCK_COUNT; p++)
		{
			y = pkt.block[(p + 1) & (XXTEA_BLOCK_COUNT - 1)];
			z = pkt.block[p] += protocol_mx ();
		}
	}

	protocol_fix_endianess ();
}

void
main (void)
{
	uint8_t j;
	uint16_t crc;

	/* configure CPU peripherals */
	OSCCON = CONFIG_CPU_OSCCON_SLOW;
	OPTION_REG = CONFIG_CPU_OPTION;
	PORTA = CONFIG_CPU_PORTA;
	PORTC = CONFIG_CPU_PORTC;
	TRISA = CONFIG_CPU_TRISA;
	TRISC = CONFIG_CPU_TRISC;
	WPUA = CONFIG_CPU_WPUA;
	WPUC = CONFIG_CPU_WPUC;
	ANSELA = CONFIG_CPU_ANSELA;
	ANSELC = CONFIG_CPU_ANSELC;

	INTE = 0;
	CONFIG_PIN_SENSOR = 0;
	CONFIG_PIN_TX_POWER = 0;

	/* initalize hardware */
	timer_init ();

	/* verify RF chip */
	nRFCMD_Init ();
	while(1)
	{
		nRFCMD_Channel (23);
		if(nRFCMD_RegGet (NRF_REG_RF_CH)==23)
		{
			nRFCMD_Channel (42);
			if(nRFCMD_RegGet (NRF_REG_RF_CH)==42)
				break;
		}

		CONFIG_PIN_LED = 1;
		sleep_jiffies (JIFFIES_PER_MS (25));
		CONFIG_PIN_LED = 0;
		sleep_jiffies (JIFFIES_PER_MS (25));
	}

	/* blink to show readyiness */
	for (j = 0; j <= 10; j++)
	{
		CONFIG_PIN_LED = j & 1;
		sleep_jiffies (JIFFIES_PER_MS (25));
	}

	// get tag OID out of EEPROM or FLASH
	oid_ram =
		(((uint16_t) (eeprom_read (EEPROM_OID_H))) << 8) |
		eeprom_read (EEPROM_OID_L);
	if (oid_ram == 0xFFFF)
		oid_ram = (uint16_t) oid;
	else
		global_flags |= RFBFLAGS_OID_UPDATED;

	// increment code block after power cycle
	code_block =
		(((uint16_t) (eeprom_read (EEPROM_CODE_BLOCK_H))) << 8) |
		eeprom_read (EEPROM_CODE_BLOCK_L);
	store_incremented_codeblock ();

	// update random seed
	seq = code_block ^ oid_ram ^ seed;
	srand (crc16 ((unsigned char *) &seq, sizeof (seq)));

	// increment code blocks to make sure that seq is
	// higher or equal after battery change
	seq = ((uint32_t) (code_block - 1)) << 16;

	nRFCMD_Channel (CONFIG_TRACKER_CHANNEL);

	while (1)
	{
		// random delay to make opaque tracking based on
		// timer deviation difficult
		sleep_jiffies (JIFFIES_PER_MS (50 + (rand () % 42)));

		// populate common fields
		pkt.hdr.oid = htons (oid_ram);
		pkt.hdr.flags = global_flags;

		pkt.hdr.proto = RFBPROTO_BEACONTRACKER;
		pkt.tracker.oid_last_seen = 0;

		pkt.tracker.strength = 0;
		pkt.tracker.powerup_count = htons (code_block);
		pkt.tracker.reserved = 0;
		pkt.tracker.seq = htonl (seq);

		// add CRC to packet
		crc =
			crc16 (pkt.byte, sizeof (pkt.tracker) - sizeof (pkt.tracker.crc));
		pkt.tracker.crc = htons (crc);

		// adjust transmit strength
		nRFCMD_RegPut (NRF_REG_RF_SETUP | WRITE_REG,
					   NRF_RFOPTIONS | (pkt.tracker.strength << 1));

		// encrypt the data
		protocol_encode ();

		// transmit data to nRF24L01 chip
		nRFCMD_RegWrite (WR_TX_PLOAD | WRITE_REG, (uint8_t*)&pkt, sizeof (pkt));
		// send data away
		nRFCMD_Execute ();
		// halt RF frontend
		nRFCMD_ResetStop ();

		// update code_block so on next power up
		// the seq will be higher or equal
		// TODO: wrapping
		crc = (unsigned short) (seq >> 16);
		if (crc == code_block)
			store_incremented_codeblock ();

		// blinking pattern
		if ((seq & 0x1F) == 0)
		{
			CONFIG_PIN_LED = 1;

			// blink for 1ms
			sleep_jiffies (JIFFIES_PER_MS (1));

			// disable LED
			CONFIG_PIN_LED = 0;
		}

		// finally increase sequence number
		seq++;
	}
}
