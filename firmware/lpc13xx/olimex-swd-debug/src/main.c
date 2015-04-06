/***************************************************************
 *
 * OpenBeacon.org - Single Wire Debug USB Gateway
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
#include "usbserial.h"
#include "swd.h"

volatile uint32_t timer_old, pulse_length, pulse_count;

void
CDC_GetCommand (unsigned char *command)
{
	(void) command;
}

int
main (void)
{
	/* Initialize GPIO (sets up clock) */
	GPIOInit ();

	/* Set LED port pin to output */
	GPIOSetDir (LED_PORT, LED_PIN0, 1);
	GPIOSetDir (LED_PORT, LED_PIN1, 1);
	GPIOSetValue (LED_PORT, LED_PIN0, LED_OFF);
	GPIOSetValue (LED_PORT, LED_PIN1, LED_OFF);

	/* Init Power Management Routines */
	pmu_init ();

	/* CDC USB Initialization */
	init_usbserial ();

	/* update core clock variable */
	SystemCoreClockUpdate();

	/* init debug interface */
	swd_init();

	while(TRUE)
	{
		debug_printf( "Hello World (0x%02X)\n", swd_rx (0xA5));

		/* blink once per second */
		GPIOSetValue (LED_PORT, LED_PIN0, LED_ON);
		pmu_wait_ms(50);
		GPIOSetValue (LED_PORT, LED_PIN0, LED_OFF);
		pmu_wait_ms(50);
	}

	return 0;
}
