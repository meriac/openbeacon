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
#include <stdint.h>
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
static const uint8_t g_MacroResetStop[] = {
	0x02, NRF_REG_CONFIG | WRITE_REG, NRF_CONFIG_BYTE,
	0x01, FLUSH_TX,
	0x01, FLUSH_RX,
	0x02, NRF_REG_STATUS | WRITE_REG, 0x78,										// reset status register
	0x00
};

uint8_t
nRFCMD_XcieveByte (uint8_t byte)
{
	uint8_t idx;

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

uint8_t
nRFCMD_RegWrite (uint8_t reg, const uint8_t * buf, uint8_t count)
{
	uint8_t res;

	CONFIG_PIN_CSN = 0;
	res = nRFCMD_XcieveByte (reg);
	if (buf)
		while (count--)
			nRFCMD_XcieveByte (*buf++);
	CONFIG_PIN_CSN = 1;

	return res;
}

uint8_t
nRFCMD_RegRead (uint8_t reg, uint8_t * buf, uint8_t count)
{
	uint8_t res;

	CONFIG_PIN_CSN = 0;
	res = nRFCMD_XcieveByte (reg);
	if (buf)
		while (count--)
			*buf++ = nRFCMD_XcieveByte (0);
	CONFIG_PIN_CSN = 1;

	return res;
}

void
nRFCMD_Macro (const uint8_t * macro)
{
	uint8_t size;

	while ((size = *macro++) != 0)
	{
		nRFCMD_RegWrite (*macro, macro + 1, size - 1);
		macro += size;
	}
}

void
nRFCMD_ResetStop (void)
{
	nRFCMD_Macro (g_MacroResetStop);
}

void
nRFCMD_Execute (void)
{
	nRFCMD_Config (NRF_CONFIG_BYTE | NRF_CONFIG_PWR_UP);
	sleep_jiffies (JIFFIES_PER_MS (1.5));
	CONFIG_PIN_CE = 1;
	sleep_jiffies (JIFFIES_PER_MS (1));
	CONFIG_PIN_CE = 0;
	nRFCMD_Config (NRF_CONFIG_BYTE);
}

uint8_t
nRFCMD_RegGet (uint8_t reg)
{
	uint8_t res;

	CONFIG_PIN_CSN = 0;
	nRFCMD_XcieveByte (reg);
	res = nRFCMD_XcieveByte (0);
	CONFIG_PIN_CSN = 1;

	return res;
}

uint8_t
nRFCMD_RegExec (uint8_t reg)
{
	uint8_t res;

	CONFIG_PIN_CSN = 0;
	res = nRFCMD_XcieveByte (reg);
	CONFIG_PIN_CSN = 1;

	return res;
}

void
nRFCMD_RegPut (uint8_t reg, uint8_t value)
{
	CONFIG_PIN_CSN = 0;
	nRFCMD_XcieveByte (reg);
	nRFCMD_XcieveByte (value);
	CONFIG_PIN_CSN = 1;
}

void
nRFCMD_Channel (uint8_t channel)
{
	nRFCMD_RegPut (NRF_REG_RF_CH | WRITE_REG, channel);
}

void
nRFCMD_Config (uint8_t config)
{
	nRFCMD_RegPut (NRF_REG_CONFIG | WRITE_REG, config);
}

void
nRFCMD_Listen (uint8_t jiffies)
{
	nRFCMD_Config (NRF_CONFIG_BYTE | NRF_CONFIG_PWR_UP);
	sleep_jiffies (JIFFIES_PER_MS (1.5));
	nRFCMD_Config (NRF_CONFIG_BYTE | NRF_CONFIG_PWR_UP | NRF_CONFIG_PRIM_RX);

	CONFIG_PIN_CE = 1;
	sleep_jiffies (jiffies);
	CONFIG_PIN_CE = 0;

	/* keep powered after IRQ */
	nRFCMD_Config (CONFIG_PIN_IRQ ? NRF_CONFIG_BYTE : NRF_CONFIG_BYTE |
				   NRF_CONFIG_PWR_UP);
}

void
nRFCMD_Tx (const uint8_t * buf, uint8_t count, uint8_t jiffies)
{
	/* upload data */
	nRFCMD_RegWrite (WR_TX_PLOAD, buf, count);

	/* transmit data */
	CONFIG_PIN_CE = 1;
	sleep_jiffies (jiffies);
	CONFIG_PIN_CE = 0;
}

void
nRFCMD_Init (void)
{
	/* update CPU default pin settings */
	CONFIG_PIN_CSN = 1;
	CONFIG_PIN_CE = 0;
	CONFIG_PIN_MOSI = 0;
	CONFIG_PIN_SCK = 0;

	sleep_jiffies (JIFFIES_PER_MS (2));

	/* send initialization macro to RF chip */
	nRFCMD_Macro (g_MacroInitialization);
}
