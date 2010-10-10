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

void
spi_init_pin (spi_cs chipselect)
{
  GPIOSetDir (
	(uint8_t)(chipselect >> 24),
	(uint8_t)(chipselect >> 16),
	1
  );
}

int
spi_txrx (spi_cs chipselect, const void *tx, void *rx, uint32_t len)
{
  uint8_t data;

  /* activate chip select */
  GPIOSetValue ((uint8_t)(chipselect >> 24), (uint8_t)(chipselect >> 16), chipselect & SPI_CS_MODE_INVERT_CS);

  while(len--)
  {
    if(tx)
    {
	data = *((uint8_t*)tx);
	tx = ((uint8_t*)tx)+1;
	if(chipselect & SPI_CS_MODE_BIT_REVERSED)
	    data = BIT_REVERSE (data);
    }
    else
	data = 0;

    /* wait till ready */
    while ((LPC_SSP->SR & 0x02) == 0);
    LPC_SSP->DR = data;

    /* wait till sent */
    while ((LPC_SSP->SR & 0x04) == 0);
    data = LPC_SSP->DR;

    if(rx)
    {
	if(chipselect & SPI_CS_MODE_BIT_REVERSED)
	    data = BIT_REVERSE (data);
	*((uint8_t*)rx) = data;
	rx = ((uint8_t*)rx)+1;
    }
  }

  /* de-activate chip select */
  GPIOSetValue ((uint8_t)(chipselect >> 24), (uint8_t)(chipselect >> 16), (chipselect & SPI_CS_MODE_INVERT_CS) ^ SPI_CS_MODE_INVERT_CS );

  return 0;
}

void
spi_status (void)
{
    debug_printf("\nSPI Status: SYSCLK:%uHz\n",SystemCoreClock);
}

void
spi_init (void)
{
  /* reset SSP peripheral */
  LPC_SYSCON->PRESETCTRL = 0x01;

  /* Enable SSP clock */
  LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 11);

  // Enable SSP peripheral
  LPC_IOCON->PIO0_8 = 0x01 | (0x01 << 3);	/* MISO, Pulldown */
  LPC_IOCON->PIO0_9 = 0x01;	/* MOSI */
  LPC_IOCON->SCKLOC = 0x00;	/* route to PIO0_10 */
  LPC_IOCON->JTAG_TCK_PIO0_10 = 0x02;	/* SCK */

  /* Set SSP PCLK to 4.5MHz
     DIV=1 */
  LPC_SYSCON->SSPCLKDIV = 0x01;
  /* 8 bit, SPI, SCR=0 */
  LPC_SSP->CR0 = 0x0007;
  LPC_SSP->CR1 = 0x0002;
  /* SSP0 Clock Prescale Register
     CPSDVSR = 2 */
  LPC_SSP->CPSR = 0x02;
}
