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

__CONFIG (0x02E4);
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

static u_int8_t
read_adc (void)
{
	u_int8_t res;

	/* enable ADC */
	ADCON0 = 1;
	/* configure RA0 as analog input */
	TRISA0 = 1;
	ANS0 = 1;
	/* wait for ADC to initialize (-1ms) */
	sleep_deep (SLEEPDEEP_4ms);

	/* enable sensor power */
	CONFIG_PIN_SENSOR_POWER = 1;

	/* wait for power to settle */
	sleep_deep (SLEEPDEEP_1ms);
	/* ADC conversion */
	GO_DONE = 1;
	while (GO_DONE);
	res = ADRESH;

	/* disable sensor power */
	CONFIG_PIN_SENSOR_POWER = 0;

	/* configure RA0 as digital output */
	TRISA0 = 0;
	ANS0 = 0;
	/* disable ADC */
	ADCON0 = 0;

	return res;
}

void
main (void)
{
	u_int8_t i, status, last_data, tx_count;
	s_int16_t data;
	u_int16_t crc;

	/* configure CPU peripherals */
	OPTION_REG = CONFIG_CPU_OPTION;
	PORTA = CONFIG_CPU_PORTA;
	TRISA = CONFIG_CPU_TRISA;
	TRISC = CONFIG_CPU_TRISC;
	WPUA = CONFIG_CPU_WPUA;
	ANSEL = CONFIG_CPU_ANSEL;
	ADCON1 = CONFIG_CPU_ADCON1;
	CMCON0 = CONFIG_CPU_CMCON0;
	OSCCON = CONFIG_CPU_OSCCON;

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

	last_data = 0;
	tx_count = 0;
	i = 0;
	while (1)
	{
		i++;

		data = read_adc ();

		if ((abs (((signed short) last_data) - data) >= ADC_TX_TRESHOLD)
			&& (data >= ADC_TX_MIN) && (data <= ADC_TX_MAX))
		{
			tx_count = 5;
			CONFIG_PIN_LED = 1;
			sleep_deep (SLEEPDEEP_1ms);
			CONFIG_PIN_LED = 0;
		}

		if (tx_count || ((i & 0x7F) == 0))
		{
			if (tx_count)
				tx_count--;
			else
			{
				CONFIG_PIN_LED = 1;
				sleep_deep (SLEEPDEEP_1ms);
				CONFIG_PIN_LED = 0;
			}

			g_MacroBeacon.rf_setup =
				NRF_RFOPTIONS | (CONFIG_TX_POWER_LEVEL << 1);
			g_MacroBeacon.env.pkt.hdr.proto = RFBPROTO_BEACONTRACKER;
			g_MacroBeacon.env.pkt.hdr.flags = 0;
			g_MacroBeacon.env.pkt.hdr.oid = htons ((u_int16_t) oid);
			g_MacroBeacon.env.pkt.oid_last_seen = 0;
			g_MacroBeacon.env.pkt.powerup_count =
				htons ((u_int16_t) code_block);
			g_MacroBeacon.env.pkt.strength = CONFIG_TX_POWER_LEVEL;
			g_MacroBeacon.env.pkt.seq = htonl (seq++);
			g_MacroBeacon.env.pkt.reserved = data;

			/* remember last data transmitted */
			last_data = data;

			crc = crc16 (g_MacroBeacon.env.datab,
						 sizeof (g_MacroBeacon.env.pkt) -
						 sizeof (g_MacroBeacon.env.pkt.crc));
			g_MacroBeacon.env.pkt.crc = htons (crc);

			// update code_block so on next power up
			// the seq will be higher or equal
			crc = seq >> 16;
			if (crc == code_block)
				store_incremented_codeblock ();

			// encrypt my data
			shuffle_tx_byteorder ();
			xxtea_encode ();
			shuffle_tx_byteorder ();

			// random backoff before transmitting
			sleep_jiffies (JIFFIES_PER_MS (37) + (rand () & 0x7FF));

			// send it away
			nRFCMD_Macro ((unsigned char *) &g_MacroBeacon);
			nRFCMD_Execute ();
		}
		else
			sleep_deep (SLEEPDEEP_132ms);
	}
}
