/***************************************************************
 *
 * OpenBeacon.org - RGB Strip control
 *
 * Copyright 2014 Milosch Meriac <milosch@meriac.com>
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
#include "cie1931.h"
#include "image.h"

#define LED_COUNT (64)
#define SPI_CS_RGB SPI_CS(LED_PORT,LED_PIN1,6, SPI_CS_MODE_NORMAL )

static const uint8_t g_latch = 0;
static uint8_t g_data[LED_COUNT][3];

void
CDC_GetCommand (unsigned char *command)
{
	(void) command;
}

void update_leds(void)
{
	/* transmit new values */
	spi_txrx (SPI_CS_RGB, &g_data, sizeof(g_data), NULL, 0);

	/* latch data */
	memset(&g_data, 0, sizeof(g_data));
	spi_txrx (SPI_CS_RGB, &g_data, sizeof(g_data), NULL, 0);
}

int
main (void)
{
	int height,y,x,t;
	const uint8_t *p;
	uint16_t data;

	/* Initialize GPIO (sets up clock) */
	GPIOInit ();

	/* Set LED port pin to output */
	GPIOSetDir (LED_PORT, LED_PIN0, 1);
	GPIOSetValue (LED_PORT, LED_PIN0, LED_OFF);

	/* Init Power Management Routines */
	pmu_init ();

	/* setup SPI chipselect pin */
	spi_init ();
	spi_init_pin (SPI_CS_RGB);

	/* calculate height */
	height = image.height<LED_COUNT ? image.height : LED_COUNT;

	if(image.bytes_per_pixel!=2)
		while(1);

	while(TRUE)
	{
#ifdef TRIGGER_RECORDING
		/* blink once to trigger exposure */
		memset(&g_data, 0xFF, sizeof(g_data));
		update_leds();
		pmu_wait_ms(50);

		/* switch back to black */
		memset(&g_data, 0x80, sizeof(g_data));
		update_leds();
		pmu_wait_ms(500);
#endif/*TRIGGER_RECORDING*/

		/* transmit stored image */
		for(x=0; x<(int)image.width; x++)
		{
			for(y=0; y<height; y++)
			{
				p = &image.pixel_data[(y*image.width+x)*2];
				data = (((uint16_t)p[1])<<8) | p[0];
				t = LED_COUNT - y; 
				g_data[t][1] = g_cie[(data & 0xF800UL)>>9];
				g_data[t][2] = g_cie[(data & 0x07E0UL)>>4];
				g_data[t][0] = g_cie[(data & 0x001FUL)<<1];
			}

			/* send data */
			update_leds();
			pmu_wait_ms(5);
		}

		/* turn off LED's */
		memset(&g_data, 0x80, sizeof(g_data));
		update_leds();

#ifdef TRIGGER_RECORDING
		pmu_wait_ms(2000);
#endif/*TRIGGER_RECORDING*/
	}
	return 0;
}

