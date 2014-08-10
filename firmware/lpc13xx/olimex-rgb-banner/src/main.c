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

#define TRIGGER_RECORDING

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
	int height,y,x,count;
	const uint8_t *p;
	uint8_t t,value,*out,line[IMAGE_HEIGHT/2];

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
	height = IMAGE_HEIGHT<LED_COUNT ? IMAGE_HEIGHT : LED_COUNT;

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
		pmu_wait_ms(1000);
#endif/*TRIGGER_RECORDING*/

		/* transmit stored image */
		for(x=0; x<IMAGE_WIDTH; x++)
		{
			/* get current line */
			p = &g_img_lines[g_img_lookup_line[x]];

			count = 0;
			out = line;
			while((t = *p++) != 0xF0)
			{
				/* run length decoding case */
				if((t & 0xF0)==0xF0)
				{
					t &= 0xF;
					count += t;
					value = *p++;
					while(t--)
						*out++ = value;
				}
				/* single character */
				else
				{
					*out++ = t;
					count++;
				}
			}

			/* 4 bit unpacking */
			p = line;
			for(y=0; y<(height/2); y++)
			{
				t = *p++;

				value = g_cie[(((t>>0) & 0xF) * IMAGE_VALUE_MULTIPLIER)>>1];
				memset(&g_data[y*2+0], value, 3);

				value = g_cie[(((t>>4) & 0xF) * IMAGE_VALUE_MULTIPLIER)>>1];
				memset(&g_data[y*2+1], value, 3);
			}

			/* send data */
			update_leds();
			pmu_wait_ms(5);
		}

		/* turn off LED's */
		memset(&g_data, 0x80, sizeof(g_data));
		update_leds();

#ifdef TRIGGER_RECORDING
		while(TRUE)
			pmu_wait_ms(1000);
#endif/*TRIGGER_RECORDING*/
	}
	return 0;
}

