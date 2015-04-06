/***************************************************************
 *
 * OpenBeacon.org - Single Wire Debug support
 *
 * Copyright 2015 Milosch Meriac <milosch@meriac.com>
 *
 ***************************************************************

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
#include <openbeacon.h>
#include "spi.h"

#define BIT_REVERSE(x) ((unsigned char)(__RBIT(x)>>24))

int
swd_rx (uint8_t cmd)
{
	uint32_t res;
	uint8_t data;

	/* MOSI */
	LPC_IOCON->PIO0_9 = 0x01;

	/* 8 bit TX */
	LPC_SSP->CR0 = 0x87;
	LPC_SSP->DR = cmd;

	/* wait for RX to complete */
	while ((LPC_SSP->SR & 4) == 0);
	/* RX data dummy read */
	data = LPC_SSP->DR;

	/* set to GPIO, input & pulldown */
	LPC_IOCON->PIO0_9 = 0x01 << 3;
	/* 4 bit RX */
	LPC_SSP->CR0 = 0x83;
	/* transmit dummy byte */
	LPC_SSP->DR = 0;
	/* wait for TX to complete */
	while ((LPC_SSP->SR & 4) == 0);
	/* read ACK */
	data = LPC_SSP->DR;

	/* 16 bit TX */
	LPC_SSP->CR0 = 0x8F;
	/* send out 32 clocks */
	LPC_SSP->DR = 0;
	LPC_SSP->DR = 0;

	/* wait for RX FIFO to be filled */
	while ((LPC_SSP->SR & 4) == 0);
	res = (uint16_t)LPC_SSP->DR;
	while ((LPC_SSP->SR & 4) == 0);
	res = (res<<16) | ((uint16_t)LPC_SSP->DR);

	/* 4 bit TX */
	LPC_SSP->CR0 = 0x83;
	/* send out 4 clocks */
	LPC_SSP->DR = 0;
	while ((LPC_SSP->SR & 4) == 0);
	data = (uint8_t)LPC_SSP->DR;

	return data;
}

void
swd_status (void)
{
	debug_printf (" * SPI: CLK:%uMHz\n",
				  (SystemCoreClock /
				   (LPC_SYSCON->SSPCLKDIV * LPC_SSP->CPSR)) / 1000000);
}

void
swd_init (void)
{
	/* reset SSP peripheral */
	LPC_SYSCON->PRESETCTRL = 0x01;

	/* Enable SSP clock */
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 11);

	/* Enable SSP peripheral MISO, Pulldown */
	LPC_IOCON->PIO0_8 = 0x01 | (0x01 << 3);

	/* MOSI, GPIO to input */
	GPIOSetDir (0, 9, 0);
	LPC_IOCON->PIO0_9 = 0x01;

	/* route to PIO0_10 */
	LPC_IOCON->SCKLOC = 0x00;
	/* SCK */
	LPC_IOCON->JTAG_TCK_PIO0_10 = 0x02;

	/* Set SSP PCLK to 48MHz DIV=1 */
	LPC_SYSCON->SSPCLKDIV = 0x01;
	LPC_SSP->CPSR = 72;

	/* 8 bit, SPI, SCR=0 */
	LPC_SSP->CR0 = 0x0007;
	LPC_SSP->CR1 = 0x0002;
}

void
swd_close (void)
{
	/* Disable SSP clock */
	LPC_SYSCON->SYSAHBCLKCTRL &= ~(1 << 11);

	/* Disable SSP PCLK */
	LPC_SYSCON->SSPCLKDIV = 0x00;

	/* Enable SSP peripheral MISO, Pulldown */
	LPC_IOCON->PIO0_8 = 0x00;
	GPIOSetDir (0, 8, 1);
	GPIOSetValue (0, 8, 0);

	/* MOSI */
	LPC_IOCON->PIO0_9 = 0x00;
	GPIOSetDir (0, 9, 1);
	GPIOSetValue (0, 9, 0);

	/* SCK */
	LPC_IOCON->JTAG_TCK_PIO0_10 = 0x01;
	GPIOSetDir (0, 10, 1);
	GPIOSetValue (0, 10, 0);
}
