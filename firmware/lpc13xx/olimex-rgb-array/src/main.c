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

#define LED_X (22L)
#define LED_Y (5L)
#define LED_COUNT (LED_X*LED_Y)
#define CIE_MAX_INDEX2 (CIE_MAX_INDEX/2)
#define SPI_CS_RGB SPI_CS(LED_PORT,LED_PIN1,6, SPI_CS_MODE_NORMAL )

typedef struct {
	uint8_t b, r ,g;
} TRGB;

static const uint8_t g_latch = 0;
static TRGB g_data[LED_Y][LED_X];

void update_leds(void)
{
	int x,y;
	TRGB data[LED_COUNT], *dst;

	/* transform array to linear LED strip */
	for(y=0; y<LED_Y; y++)
	{
		dst = &data[y*LED_X];
		if(y&1)
			memcpy(dst, &g_data[y], sizeof(g_data[y]));
		else
			for(x=(LED_X-1); x>=0; x--)
				*dst++ = g_data[y][x];
	}

	/* transmit new values */
	spi_txrx (SPI_CS_RGB, &data, sizeof(data), NULL, 0);

	/* latch data */
	memset(&data, 0, sizeof(data));
	spi_txrx (SPI_CS_RGB, &data, sizeof(data), NULL, 0);
}

int
main (void)
{
	double t;
	int x, y;
	TRGB color, *p;

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
		for(y=0; y<LED_Y; y++)
		{
			for(x=0; x<LED_X; x++)
			{
				color.r = (sin( x*0.1+cos(y*0.1+t))*CIE_MAX_INDEX2)+CIE_MAX_INDEX2;
				color.g = (cos(-y*0.2-sin(x*0.3-t))*CIE_MAX_INDEX2)+CIE_MAX_INDEX2;
				color.b = (cos( x*0.5-cos(y*0.4+t))*CIE_MAX_INDEX2)+CIE_MAX_INDEX2;

				p = &g_data[y][x];
				p->r = g_cie[color.r];
				p->g = g_cie[color.g];
				p->b = g_cie[color.b];
			}
		}
		/* send data */
		update_leds();
		pmu_wait_ms(1);

		t+=0.05;
	}
}
