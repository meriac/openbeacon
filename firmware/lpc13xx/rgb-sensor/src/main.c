/***************************************************************
 *
 * OpenBeacon.org - main file for OpenBeacon USB II Bluetooth
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
#include "usbserial.h"

int
main (void)
{
	volatile int t;

	/* wait on boot - debounce */
	for (t = 0; t < 2000000; t++);

	/* Initialize GPIO (sets up clock) */
	GPIOInit ();
	GPIOSetDir (BUT1_PORT, BUT1_PIN, 0);
	GPIOSetDir (BUT2_PORT, BUT2_PIN, 0);

	/* CDC USB Initialization */
	init_usbserial ();

	while(1)
	{
		debug_printf("Hello World\n\r");
	}

	return 0;
}
