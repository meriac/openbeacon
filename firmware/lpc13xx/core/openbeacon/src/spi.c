/***************************************************************
 *
 * OpenBeacon.org - SPI routines for LPC13xx
 *
 * Copyright 2010 Milosch Meriac <meriac@openbeacon.de>
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

void spi_init_pin(spi_cs chipselect)
{
	GPIOSetDir((uint8_t) (chipselect >> 24), (uint8_t) (chipselect >> 16), 1);
}

void spi_txrx_done(spi_cs chipselect)
{
	GPIOSetValue((uint8_t) (chipselect >> 24),
		(uint8_t) (chipselect >> 16), (chipselect
		& SPI_CS_MODE_INVERT_CS) ^ SPI_CS_MODE_INVERT_CS);
}

int spi_txrx(spi_cs chipselect, const void *tx, uint16_t txlen, void *rx,
		uint16_t rxlen)
{
	uint8_t data;
	uint16_t total, xfered;

	/* SSP0 Clock Prescale Register to SYSCLK/CPSDVSR */
	LPC_SSP->CPSR = (chipselect >> 8) & 0xFF;

	/* activate chip select */
	if ((chipselect & SPI_CS_MODE_SKIP_CS_ASSERT) == 0)
		GPIOSetValue((uint8_t) (chipselect >> 24),
				(uint8_t) (chipselect >> 16), chipselect
						& SPI_CS_MODE_INVERT_CS);

	/* calculate SPI transaction size */
	xfered = total = (chipselect & SPI_CS_MODE_SKIP_TX) ? rxlen + txlen
			: (txlen >= rxlen) ? txlen : rxlen;

	while (xfered)
	{
		if (total)
		{
			total--;

			if (txlen)
			{
				txlen--;
				data = *((uint8_t *) tx);
				tx = ((uint8_t *) tx) + 1;
				if (chipselect & SPI_CS_MODE_BIT_REVERSED)
					data = BIT_REVERSE (data);
			}
			else
				data = 0;

			/* Wait if TX fifo is full */
			while((LPC_SSP->SR & 2)==0);

			LPC_SSP->DR = data;
		}

		while (LPC_SSP->SR & 0x04)
		{
			data = LPC_SSP->DR;
			xfered--;

			/* skip txlen in output if SPI_CS_MODE_SKIP_TX */
			if (rxlen && (xfered < rxlen))
			{
				rxlen--;
				if (chipselect & SPI_CS_MODE_BIT_REVERSED)
					data = BIT_REVERSE (data);
				*((uint8_t *) rx) = data;
				rx = ((uint8_t *) rx) + 1;
			}
		}
	}

	/* Wait for TX transaction to finish */
	while((LPC_SSP->SR & 1)==0);

	/* de-activate chip select */
	if ((chipselect & SPI_CS_MODE_SKIP_CS_DEASSERT) == 0)
		spi_txrx_done (chipselect);
	return 0;
}

void spi_status(void)
{
	debug_printf(" * SPI: CLK:%uMHz\n", (SystemCoreClock / (LPC_SYSCON->SSPCLKDIV * LPC_SSP->CPSR))
			/ 1000000);
}

void spi_init(void)
{
	/* reset SSP peripheral */
	LPC_SYSCON->PRESETCTRL = 0x01;

	/* Enable SSP clock */
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 11);

	/* Enable SSP peripheral MISO, Pulldown */
	LPC_IOCON->PIO0_8 = 0x01 | (0x01 << 3);
	/* MOSI */
	LPC_IOCON->PIO0_9 = 0x01;
	/* route to PIO0_10 */
	LPC_IOCON->SCKLOC = 0x00;
	/* SCK */
	LPC_IOCON->JTAG_TCK_PIO0_10 = 0x02;

	/* Set SSP PCLK to 48MHz DIV=1 */
	LPC_SYSCON->SSPCLKDIV = 0x01;
	/* 8 bit, SPI, SCR=0 */
	LPC_SSP->CR0 = 0x0007;
	LPC_SSP->CR1 = 0x0002;
}

void spi_close(void)
{
	/* Disable SSP clock */
	LPC_SYSCON->SYSAHBCLKCTRL &= ~(1 << 11);

	/* Disable SSP PCLK */
	LPC_SYSCON->SSPCLKDIV = 0x00;

	/* Enable SSP peripheral MISO, Pulldown */
	LPC_IOCON->PIO0_8 = 0x00;
	GPIOSetDir   (0, 8, 1); // OUT
	GPIOSetValue (0, 8, 0);

	/* MOSI */
	LPC_IOCON->PIO0_9 = 0x00;
	GPIOSetDir   (0, 9, 1); // OUT
	GPIOSetValue (0, 9, 0);

	/* SCK */
	LPC_IOCON->JTAG_TCK_PIO0_10 = 0x01;
	GPIOSetDir   (0, 10, 1); // OUT
	GPIOSetValue (0, 10, 0);
}
