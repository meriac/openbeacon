/*
    This file is part of the SocioPatterns firmware
    Copyright (C) 2008 Istituto per l'Interscambio Scientifico I.S.I.
    You can contact us by email (isi@isi.it) or write to:
    ISI Foundation, Viale S. Severo 65, 10133 Torino, Italy. 

    This program was written by Ciro Cattuto <ciro.cattuto@gmail.com>

    This program is based on:
    OpenBeacon.org - main entry, CRC, behaviour
    Copyright 2006 Milosch Meriac <meriac@openbeacon.de>

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

    This file was modified on: July 1st, 2008
*/

#include <htc.h>
#include <stdlib.h>
#include "config.h"
#include "timer.h"
#include "nRF_CMD.h"
#include "nRF_HW.h"
#include "xxtea.h"
#include "main.h"

__CONFIG (0x03d4);
__EEPROM_DATA (0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
__EEPROM_DATA (0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

static u_int16_t oid;
static u_int32_t seq;
static u_int16_t oid_seen[SEEN_MAX], oid_last_seen;
static u_int8_t seen_count = 0;


TMacroBeacon g_MacroBeacon = {
	0x02,	// Size
	RF_SETUP | WRITE_REG,	// Setup RF Options
	RFB_RFOPTIONS,

	0x02, RF_CH | WRITE_REG,
	CONFIG_DEFAULT_CHANNEL,

	0x06, TX_ADDR | WRITE_REG,
	0x01, 0x02, 0x03, 0x02, 0x01,

	sizeof (TBeaconEnvelope) + 1,	// Size
	WR_TX_PLOAD | WRITE_REG	// Transmit Packet Opcode
};

unsigned long
htonl (unsigned long src)
{
	unsigned long res;

	((unsigned char *) &res)[0] = ((unsigned char *) &src)[3];
	((unsigned char *) &res)[1] = ((unsigned char *) &src)[2];
	((unsigned char *) &res)[2] = ((unsigned char *) &src)[1];
	((unsigned char *) &res)[3] = ((unsigned char *) &src)[0];

	return res;
}

unsigned short
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

void
shuffle_tx_byteorder (void)
{
	unsigned char tmp;

	SHUFFLE (0 +  0, 3 +  0);
	SHUFFLE (1 +  0, 2 +  0);
	SHUFFLE (0 +  4, 3 +  4);
	SHUFFLE (1 +  4, 2 +  4);
	SHUFFLE (0 +  8, 3 +  8);
	SHUFFLE (1 +  8, 2 +  8);
	SHUFFLE (0 + 12, 3 + 12);
	SHUFFLE (1 + 12, 2 + 12);
}

unsigned short
crc16 (const unsigned char *buffer, unsigned char size)
{
	unsigned short crc = 0xFFFF;

	if (buffer) {
		while (size--) {
			crc = (crc >> 8) | (crc << 8);
			crc ^= *buffer++;
			crc ^= ((unsigned char) crc) >> 4;
			crc ^= crc << 12;
			crc ^= (crc & 0xFF) << 5;
		}
    }

  return crc;
}


void
main (void)
{
	u_int8_t i, j, status;
	u_int16_t crc;
	u_int8_t clicked = 0;
	unsigned short delta;

	timer1_init ();
	nRFCMD_Init ();

	for (i = 0; i <= 50; i++)
	{
      CONFIG_PIN_LED = i & 1;
      sleep_jiffies (25 * TIMER1_JIFFIES_PER_MS);
    }

	nRFCMD_Stop ();

	// get tag ID
	((unsigned char *) &oid)[0] = EEPROM_READ (0);
	((unsigned char *) &oid)[1] = EEPROM_READ (1);

	// get seed for random generator
	((unsigned char *) &seq)[0] = EEPROM_READ (2);
	((unsigned char *) &seq)[1] = EEPROM_READ (3);
	((unsigned char *) &seq)[2] = EEPROM_READ (4);
	((unsigned char *) &seq)[3] = EEPROM_READ (5);

	/* set seed of random generator */
	srand (crc16 ((unsigned char *) &seq, sizeof (seq)));

	i = 0;
	while (1)
	{

		// --- SLEEP ---
		sleep_jiffies (0x0400 + (rand() >> 6));

		// --- SWITCH TO SLEEP/RECEIVE MODE ---
		nRFCMD_StartRX ();

		delta = 0x80; // time left to sleep while receiving
		do {
			CONFIG_PIN_CE = 1;
			delta = sleep_jiffies (delta);
			CONFIG_PIN_CE = 0;

			while ( (nRFCMD_RegGet (FIFO_STATUS) & NRF_FIFO_RX_EMPTY) == 0 )
			{
				// receive packet
				nRFCMD_RegRead (RD_RX_PLOAD, g_MacroBeacon.env.datab, sizeof (g_MacroBeacon.env.datab));
				nRFCMD_RegReadWrite (STATUS | WRITE_REG, NRF_CONFIG_MASK_RX_DR);

				#ifdef OPENBEACON_ENABLEENCODE
				shuffle_tx_byteorder ();
				xxtea_decode ();
				shuffle_tx_byteorder ();
				#endif

				// verify the crc checksum
				crc = crc16 (g_MacroBeacon.env.datab,
					sizeof (g_MacroBeacon.env.pkt) - sizeof (g_MacroBeacon.env.pkt.crc));

				if (htons (crc) == g_MacroBeacon.env.pkt.crc) 
					{
					oid_last_seen = htons (g_MacroBeacon.env.pkt.oid);
					// set the highest bit of oid according to whether
					// it was a contact at power 0 (bit clear) or power 1 (bit set)
					if (g_MacroBeacon.env.pkt.strength)
						oid_last_seen |= 0x8000;

					// have we already seen oid ?
					for (j=0; (j<seen_count) && ((oid_seen[j] ^ oid_last_seen) << 1); j++);
					// if we have seen it at power 0, ignore contact at power 1,
					// otherwise add to the list of seen tags (up to a max of SEEN_MAX)
					if (j < seen_count)
						oid_seen[j] &= (oid_last_seen | 0x7fff);
					else if (seen_count < SEEN_MAX)
						oid_seen[seen_count++] = oid_last_seen;
					}
				}

		} while (delta != 0xffff);	// keep going until the timer stops us


		// --- SWITCH TO TRANSMIT MODE ---

		if (i & 0x07) {
			g_MacroBeacon.rf_setup = NRF_RFOPTIONS | (((i & 0x07) < 6) ? 0 : 0x02);
			g_MacroBeacon.rf_ch = CONFIG_CONTACT_CHANNEL;
			g_MacroBeacon.env.pkt.hdr.proto = RFBPROTO_CONTACTTRACKER;
			g_MacroBeacon.rf_tx_addr[0] = 'O';
			g_MacroBeacon.rf_tx_addr[1] = 'C';
			g_MacroBeacon.rf_tx_addr[2] = 'A';
			g_MacroBeacon.rf_tx_addr[3] = 'E';
			g_MacroBeacon.rf_tx_addr[4] = 'B';
		} else {
			g_MacroBeacon.rf_setup = NRF_RFOPTIONS | (i >> 2) & 0x06;
			g_MacroBeacon.rf_ch = CONFIG_DEFAULT_CHANNEL;
			g_MacroBeacon.env.pkt.hdr.proto = RFBPROTO_BEACONTRACKER;
			g_MacroBeacon.rf_tx_addr[0] = 0x01;
			g_MacroBeacon.rf_tx_addr[1] = 0x02;
			g_MacroBeacon.rf_tx_addr[2] = 0x03;
			g_MacroBeacon.rf_tx_addr[3] = 0x02;
			g_MacroBeacon.rf_tx_addr[4] = 0x01;
			g_MacroBeacon.env.pkt.seq = htonl (seq++);
			g_MacroBeacon.env.pkt.oid_last_seen = htons (oid_last_seen);
			oid_last_seen = 0;
		}

		g_MacroBeacon.env.pkt.hdr.size = sizeof (TBeaconTracker);
		g_MacroBeacon.env.pkt.oid = htons (oid);
		g_MacroBeacon.env.pkt.flags = clicked ? RFBFLAGS_SENSOR : 0;
		g_MacroBeacon.env.pkt.strength = (g_MacroBeacon.rf_setup >> 1) & 0x03;
		g_MacroBeacon.env.pkt.reserved = 0;

		if (g_MacroBeacon.env.pkt.strength < 3 || seen_count) {
			if (g_MacroBeacon.env.pkt.strength == 3)
			{
				/* g_MacroBeacon.env.pkt.hdr.size = sizeof (TBeaconContact); */
				g_MacroBeacon.env.pkt.hdr.proto = RFBPROTO_CONTACTREPORT;
				g_MacroBeacon.env.pkt_contact.seq = htons (seq & 0xffff); // only the low 16 bits of seq

			for (j=0; j<SEEN_MAX; j++)
				g_MacroBeacon.env.pkt_contact.oid_contact[j] = (j < seen_count) ? htons(oid_seen[j]) : 0;

			seen_count = 0;
			}

		crc = crc16 (g_MacroBeacon.env.datab,
			sizeof (g_MacroBeacon.env.pkt) - sizeof (g_MacroBeacon.env.pkt.crc));
		g_MacroBeacon.env.pkt.crc = htons (crc);

		#ifdef OPENBEACON_ENABLEENCODE
		shuffle_tx_byteorder ();
		xxtea_encode ();
		shuffle_tx_byteorder ();
		#endif

		// transmit packet
		nRFCMD_Macro ((unsigned char *) &g_MacroBeacon);
		if ((i & 0x1F) == 0)
			CONFIG_PIN_LED = 1;

		nRFCMD_Execute ();

		if (((i & 0x1F) == 0) && (!clicked))
			CONFIG_PIN_LED = 0;
		}

		// check for click
		if (!CONFIG_PIN_SENSOR) {
			clicked = 10;
			// reset touch sensor pin
			TRISA = CONFIG_CPU_TRISA & ~0x02;
			CONFIG_PIN_SENSOR = 0;
			TRISA = CONFIG_CPU_TRISA;
		}

		// handle click
		if (clicked > 0) {
			clicked--;
			if (!clicked) {
				TRISA = CONFIG_CPU_TRISA & ~0x02;
				CONFIG_PIN_SENSOR = 1;
				TRISA = CONFIG_CPU_TRISA;
			}
		}

		i++;
	}
}

