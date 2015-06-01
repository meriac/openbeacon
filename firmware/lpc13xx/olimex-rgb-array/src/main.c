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

#define LED_COUNT (22*5)
#define CIE_MAX_INDEX2 (CIE_MAX_INDEX/2)
#define SPI_CS_RGB SPI_CS(LED_PORT,LED_PIN1,6, SPI_CS_MODE_NORMAL )

static const uint8_t g_latch = 0;
static uint8_t g_data[LED_COUNT][3];

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
	double t,y;
	int i,rgb[3];

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

	/* transmit image */
	t = 0;
	while(1)
	{		
		y=t;
		for(i=0; i<LED_COUNT; i++)
		{
			rgb[0] = (sin( y*0.1+cos(y*0.01))*CIE_MAX_INDEX2)+CIE_MAX_INDEX2;
			rgb[1] = (cos(-y*0.2-sin(y*0.3 ))*CIE_MAX_INDEX2)+CIE_MAX_INDEX2;
			rgb[2] = (cos( y*0.5-cos(y*0.4 ))*CIE_MAX_INDEX2)+CIE_MAX_INDEX2;

			g_data[i][1] = g_cie[rgb[0]];
			g_data[i][2] = g_cie[rgb[1]];
			g_data[i][0] = g_cie[rgb[2]];

			y += 0.5;
		}

		/* send data */
		update_leds();
		pmu_wait_ms(1);

		t+=0.1;
	}
}
