/***************************************************************
 *
 * OpenBeacon.org - eMeter demo
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
#include <math.h>
#include "usbserial.h"

#define LED_COUNT (6*32)
#define SPI_CS_RGB SPI_CS(LED_PORT,LED_PIN1,6, SPI_CS_MODE_NORMAL )

static const uint8_t g_latch = 0;
static uint8_t g_data[LED_COUNT][3];

void
CDC_GetCommand (unsigned char *command)
{
	(void) command;
}

int
main (void)
{
	int i,t,offset;

	/* Initialize GPIO (sets up clock) */
	GPIOInit ();

	/* Set LED port pin to output */
	GPIOSetDir (LED_PORT, LED_PIN0, 1);
	GPIOSetValue (LED_PORT, LED_PIN0, LED_OFF);

	/* Init Power Management Routines */
	pmu_init ();

	/* CDC USB Initialization */
	init_usbserial ();

	/* setup SPI chipselect pin */
	spi_init ();
	spi_init_pin (SPI_CS_RGB);

	offset = t = 0;
	while(TRUE)
	{
		for(i=0; i<LED_COUNT; i++)
		{
			t = i+offset;
			g_data[i][0] = 0x80|(uint8_t)(sin(t/12.0)*16+16);
			g_data[i][1] = 0x80|(uint8_t)(cos(t/7.0)*16+16);
			g_data[i][2] = 0x80|(uint8_t)(sin(t/15.0)*16+16);
		}
		offset++;

		/* send data */
		spi_txrx (SPI_CS_RGB, &g_data, sizeof(g_data), NULL, 0);
		spi_txrx (SPI_CS_RGB, &g_latch, sizeof(g_latch), NULL, 0);
	}

	return 0;
}
