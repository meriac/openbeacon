/***************************************************************
 *
 * OpenBeacon.org - nRF24L01 hardware support / macro functions
 *
 * Copyright 2006 Milosch Meriac <meriac@openbeacon.de>
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
#include "config.h"
#include "openbeacon.h"
#include "timer.h"
#include "nRF_HW.h"
#include "nRF_CMD.h"

#assert NRF_MAC_SIZE==5

#define NRF_CONFIG_BYTE (NRF_CONFIG_EN_CRC)

// first byte payload size+1, second byte register, 3..n-th byte payload
static const unsigned char g_MacroInitialization[] = {
	0x01, OP_NOP,
	0x02, NRF_REG_CONFIG | WRITE_REG, 0x00,										// stop nRF
	0x02, NRF_REG_EN_AA | WRITE_REG, 0x00,										// disable ShockBurst(tm)
	0x02, NRF_REG_EN_RXADDR | WRITE_REG, 0x01,									// enable RX pipe address 0
	0x02, NRF_REG_SETUP_AW | WRITE_REG, NRF_MAC_SIZE - 2,						// setup MAC address width to NRF_MAC_SIZE
	0x02, NRF_REG_STATUS | WRITE_REG, 0x78,										// reset status register
	0x01, FLUSH_TX,
	0x01, FLUSH_RX,
	0x06, NRF_REG_TX_ADDR | WRITE_REG, 0x01, 0x02, 0x03, 0x02, 0x01,			// set TX_ADDR
	0x06, NRF_REG_RX_ADDR_P0 | WRITE_REG, 0x01, 0x02, 0x03, 0x02, 0x01,			// set RX_ADDR_P0 to same as TX_ADDR
	0x02, NRF_REG_RX_PW_P0 | WRITE_REG, sizeof (TBeaconEnvelope),				// set payload width of pipe 0 to sizeof(TBeaconEnvelope)
	0x02, NRF_REG_RF_CH | WRITE_REG, CONFIG_TRACKER_CHANNEL,
	0x02, NRF_REG_RF_SETUP | WRITE_REG, NRF_RFOPTIONS,							// update RF options
	0x00																		// termination
};

// first byte payload size+1, second byte register, 3..n-th byte payload
static const unsigned char g_MacroStart[] = {
	0x02, NRF_REG_CONFIG | WRITE_REG, NRF_CONFIG_BYTE | NRF_CONFIG_PWR_UP,
	0x02, NRF_REG_STATUS | WRITE_REG, 0x70,										// reset status
	0x00
};

// first byte payload size+1, second byte register, 3..n-th byte payload
static const unsigned char g_MacroStop[] = {
	0x02, NRF_REG_CONFIG | WRITE_REG, NRF_CONFIG_BYTE,
	0x02, NRF_REG_STATUS | WRITE_REG, 0x70,										// reset status
	0x00
};

// first byte payload size+1, second byte register, 3..n-th byte payload
static const unsigned char g_MacroStartRX[] = {
	0x02, NRF_REG_RF_CH | WRITE_REG, CONFIG_PROX_CHANNEL,
	0x02, NRF_REG_CONFIG | WRITE_REG,
	NRF_CONFIG_BYTE | NRF_CONFIG_PWR_UP | NRF_CONFIG_PRIM_RX,
	0x02, NRF_REG_STATUS | WRITE_REG, 0x70,										// reset status
	0x00																		// termination
};

static unsigned char
nRFCMD_XcieveByte (unsigned char byte)
{
	unsigned char idx;

	for (idx = 0; idx < 8; idx++)
	{
		CONFIG_PIN_MOSI = (byte & 0x80) ? 1 : 0;
		byte <<= 1;
		CONFIG_PIN_SCK = 1;
		byte |= CONFIG_PIN_MISO;
		CONFIG_PIN_SCK = 0;
	}
	CONFIG_PIN_MOSI = 0;

	return byte;
}

unsigned char
nRFCMD_RegWrite (unsigned char reg, const unsigned char *buf,
				 unsigned char count)
{
	unsigned char res;

	CONFIG_PIN_CSN = 0;
	res = nRFCMD_XcieveByte (reg);
	if (buf)
		while (count--)
			nRFCMD_XcieveByte (*buf++);
	CONFIG_PIN_CSN = 1;

	return res;
}

unsigned char
nRFCMD_RegRead (unsigned char reg, unsigned char *buf, unsigned char count)
{
	unsigned char res;

	CONFIG_PIN_CSN = 0;
	res = nRFCMD_XcieveByte (reg);
	if (buf)
		while (count--)
			*buf++ = nRFCMD_XcieveByte (0);
	CONFIG_PIN_CSN = 1;

	return res;
}

unsigned char
nRFCMD_RegGet (unsigned char reg)
{
	unsigned char res;

	CONFIG_PIN_CSN = 0;
	nRFCMD_XcieveByte (reg);
	res = nRFCMD_XcieveByte (0);
	CONFIG_PIN_CSN = 1;

	return res;
}

void
nRFCMD_Macro (const unsigned char *macro)
{
	unsigned char size;

	while ((size = *macro++) != 0)
	{
		nRFCMD_RegWrite (*macro, macro + 1, size - 1);
		macro += size;
	}
}

void
nRFCMD_Start (void)
{
	nRFCMD_Macro (g_MacroStart);
}

void
nRFCMD_Stop (void)
{
	nRFCMD_Macro (g_MacroStop);
}

void
nRFCMD_StartRX (void)
{
	nRFCMD_Macro (g_MacroStartRX);
	sleep_jiffies (JIFFIES_PER_MS (1));
}

void
nRFCMD_Execute (void)
{
	nRFCMD_Start ();
	sleep_2ms ();
	CONFIG_PIN_CE = 1;
	sleep_2ms ();
	CONFIG_PIN_CE = 0;
	nRFCMD_Stop ();
}

unsigned char
nRFCMD_RegExec (unsigned char reg)
{
	unsigned char res;

	CONFIG_PIN_CSN = 0;
	res = nRFCMD_XcieveByte (reg);
	CONFIG_PIN_CSN = 1;

	return res;
}

unsigned char
nRFCMD_RegReadWrite (unsigned char reg, unsigned char value)
{
	unsigned char res;

	CONFIG_PIN_CSN = 0;
	res = nRFCMD_XcieveByte (reg);
	nRFCMD_XcieveByte (value);
	CONFIG_PIN_CSN = 1;

	return res;
}

void
nRFCMD_Init (void)
{
	/* configure CPU peripherals */
	OPTION_REG = CONFIG_CPU_OPTION;
	TRISA = CONFIG_CPU_TRISA;
	TRISC = CONFIG_CPU_TRISC;
	WPUA = CONFIG_CPU_WPUA;
	ANSEL = CONFIG_CPU_ANSEL;
	CMCON0 = CONFIG_CPU_CMCON0;
	OSCCON = CONFIG_CPU_OSCCON;

	/* update CPU default pin settings */
	CONFIG_PIN_CSN = 1;
	CONFIG_PIN_CE = 0;
	CONFIG_PIN_MOSI = 0;
	CONFIG_PIN_SCK = 0;
	CONFIG_PIN_ANT_SWITCH = 0;

	sleep_2ms ();

	/* send initialization macro to RF chip */
	nRFCMD_Macro (g_MacroInitialization);
}
