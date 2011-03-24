/***************************************************************
 *
 * OpenBeacon.org - main file for OpenPCD2 basic demo
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
#include "rfid.h"

int main(void)
{
	/* Initialize GPIO (sets up clock) */
	GPIOInit();

	/* Set LED port pin to output */
	GPIOSetDir(LED_PORT, LED_BIT, LED_ON);

	/* UART setup */
	UARTInit(115200, 0);

	/* Init RFID */
	rfid_init();

	/* Update Core Clock */
	SystemCoreClockUpdate ();

	/* Start the tasks running. */
	vTaskStartScheduler();

	return 0;
}
