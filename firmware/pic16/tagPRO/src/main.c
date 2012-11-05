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

static uint16_t oid_seen[PROX_MAX], oid_last_seen;
static uint8_t oid_seen_count[PROX_MAX], oid_seen_pwr[PROX_MAX];
static uint8_t seen_count = 0;

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

static void
protocol_decode (void)
{
	protocol_fix_endianess ();

	y = pkt.block[0];
	sum = XXTEA_ROUNDS * DELTA;

	while (sum)
	{
		e = (sum >> 2) & 3;

		for (p = XXTEA_BLOCK_COUNT - 1; p >= 0; p--)
		{
			z = pkt.block[(p - 1) & (XXTEA_BLOCK_COUNT - 1)];
			y = pkt.block[p] -= protocol_mx ();
		}

		sum -= DELTA;
	}

	protocol_fix_endianess ();
}


static void
protocol_process_packet (void)
{
	uint8_t j, insert = 0;
	uint16_t tmp_oid;

	oid_last_seen = htons (pkt.hdr.oid);

	/* assign new OID if a write request has been seen for our ID */
	if ((pkt.hdr.flags & RFBFLAGS_OID_WRITE) && (oid_last_seen == oid_ram))
	{
		tmp_oid = ntohs (pkt.tracker.oid_last_seen);
		/* only write to EEPROM if the ID actually changed */
		if (tmp_oid != oid_ram)
		{
			oid_ram = tmp_oid;
			eeprom_write (EEPROM_OID_L, (uint8_t) (oid_ram));
			eeprom_write (EEPROM_OID_H, (uint8_t) (oid_ram >> 8));
			global_flags |= RFBFLAGS_OID_UPDATED;
		}

		/* reset counters */
		seq = 0;
		code_block = 0;
		store_incremented_codeblock ();

		/* confirmation-blink for 0.3s */
		CONFIG_PIN_LED = 1;
		sleep_jiffies (JIFFIES_PER_MS (100));
		CONFIG_PIN_LED = 0;
		sleep_jiffies (JIFFIES_PER_MS (100));
		CONFIG_PIN_LED = 1;
		sleep_jiffies (JIFFIES_PER_MS (100));
		CONFIG_PIN_LED = 0;

		return;
	}

	/* ignore OIDs outside of mask */
	if (oid_last_seen > PROX_TAG_ID_MASK)
		return;

	for (j = 0; (j < seen_count) && (oid_seen[j] != oid_last_seen); j++);

	if (j < seen_count)
	{

		if (pkt.tracker.strength < oid_seen_pwr[j])
			insert = 1;
		else if ((pkt.tracker.strength == oid_seen_pwr[j])
				 && (oid_seen_count[j] < PROX_TAG_COUNT_MASK))
			oid_seen_count[j]++;

	}
	else
	{

		if (seen_count < PROX_MAX)
		{
			insert = 1;
			seen_count++;
		}
		else
		{
			for (j = 0; j < PROX_MAX; j++)
				if (pkt.tracker.strength < oid_seen_pwr[j])
				{
					insert = 1;
					break;
				}
		}
	}

	if (insert)
	{
		oid_seen[j] = oid_last_seen;
		oid_seen_count[j] = 1;
		oid_seen_pwr[j] = (pkt.tracker.strength > PROX_TAG_STRENGTH_MASK) ?
			PROX_TAG_STRENGTH_MASK : pkt.tracker.strength;
	}
}

void
main (void)
{
	uint8_t j, strength;
	uint16_t crc;
	uint8_t clicked = 0;

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

	while (1)
	{
		// disable antenna dampening
		CONFIG_PIN_TX_POWER = 0;


		// random delay to make opaque tracking based on
		// timer deviation difficult
		sleep_jiffies (JIFFIES_PER_MS (50 + (rand () % 42)));

		// reset touch sensor pin
		TRISA = CONFIG_CPU_TRISA | 0x02;
		WPUA = CONFIG_CPU_WPUA | 0x02;

		sleep_jiffies (JIFFIES_PER_MS (1));

		// handle click
		if (!CONFIG_PIN_SENSOR)
			clicked = 16;
		else
			if (clicked > 0)
				clicked--;

		// reset touch sensor
		TRISA = CONFIG_CPU_TRISA;
		WPUA = CONFIG_CPU_WPUA;

		// perform RX
		if (((uint8_t) seq) & 1)
		{
			nRFCMD_Channel (CONFIG_PROX_CHANNEL);
			nRFCMD_Listen (JIFFIES_PER_MS (5));

			if (!CONFIG_PIN_IRQ)
			{
				while ((nRFCMD_RegGet (NRF_REG_FIFO_STATUS) &
						NRF_FIFO_RX_EMPTY) == 0)
				{
					// receive raw data
					nRFCMD_RegRead (RD_RX_PLOAD, pkt.byte, sizeof (pkt.byte));
					nRFCMD_RegPut (NRF_REG_STATUS | WRITE_REG,
								   NRF_CONFIG_MASK_RX_DR);

					// decrypt data
					protocol_decode ();

					// verify the crc checksum
					crc = crc16 (pkt.byte,
						sizeof (pkt.tracker) - sizeof (pkt.tracker.crc));

					// only handle RFBPROTO_PROXTRACKER packets
					if (htons (crc) == pkt.tracker.crc
						&& (pkt.hdr.proto == RFBPROTO_PROXTRACKER))
						protocol_process_packet ();
				}
				nRFCMD_ResetStop ();
			}
		}

		// populate common fields
		pkt.hdr.oid = htons (oid_ram);
		pkt.hdr.flags = global_flags;
		if (clicked)
			pkt.hdr.flags |= RFBFLAGS_SENSOR;

		// perform TX
		if (((uint8_t) seq) & 1)
		{
			CONFIG_PIN_TX_POWER = 1;
			strength = (seq & 2) ? 1 : 2;
			pkt.hdr.proto = RFBPROTO_PROXTRACKER;
			pkt.tracker.oid_last_seen = 0;
		}
		else
		{
			nRFCMD_Channel (CONFIG_TRACKER_CHANNEL);
			strength = (((unsigned char) seq) >> 1) & 0x03;

			pkt.hdr.proto = RFBPROTO_BEACONTRACKER;
			pkt.tracker.oid_last_seen = htons (oid_last_seen);
			oid_last_seen = 0;
		}

		// for lower strength send normal tracking packet
		if (strength < 3)
		{
			pkt.tracker.strength = strength;
			pkt.tracker.powerup_count = htons (code_block);
			pkt.tracker.reserved = 0;
			pkt.tracker.seq = htonl (seq);
		}
		else
			/* for highest strength send proximity report */
			{
				pkt.hdr.proto = RFBPROTO_PROXREPORT_EXT;
				for (j = 0; j < PROX_MAX; j++)
					pkt.prox.oid_prox[j] = (j < seen_count) ? (htons (
						(oid_seen[j]) |
						(oid_seen_count [j] << PROX_TAG_ID_BITS) |
						(oid_seen_pwr [j] << (PROX_TAG_ID_BITS+PROX_TAG_COUNT_BITS))
					)) : 0;
				pkt.prox.short_seq = htons ((uint16_t) seq);
				seen_count = 0;
			}

		// add CRC to packet
		crc =
			crc16 (pkt.byte, sizeof (pkt.tracker) - sizeof (pkt.tracker.crc));
		pkt.tracker.crc = htons (crc);

		// adjust transmit strength
		nRFCMD_RegPut (NRF_REG_RF_SETUP | WRITE_REG,
					   NRF_RFOPTIONS | (strength << 1));

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
		if ((seq & (clicked ? 1 : 0x1F)) == 0)
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
