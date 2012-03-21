/***************************************************************
 *
 * OpenBeacon.org - main file for OpenPCD2 libnfc interface
 *
 * Copyright 2012 Milosch Meriac <meriac@openbeacon.de>
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
#include "pmu.h"
#include "usbserial.h"

#define PN532_FIFO_SIZE 64

static uint8_t buffer_get[PN532_FIFO_SIZE+1];

inline static void
pn532_init (void)
{
	debug_printf ("Hello World 2!\n");

	/* init RFID SPI interface */
	spi_init ();

	debug_printf ("Hello World 3!\n");

	spi_init_pin (SPI_CS_PN532);

	debug_printf ("Hello World 4!\n");

	/* initialize RFID IRQ line */
	GPIOSetDir (PN532_IRQ_PORT, PN532_IRQ_PIN, 0);

	debug_printf ("Hello World 5!\n");

	/* release reset line after 400ms */
	pmu_wait_ms (400);
	GPIOSetValue (PN532_RESET_PORT, PN532_RESET_PIN, 1);
	/* wait for PN532 to boot */
	pmu_wait_ms (100);


	debug_printf ("Hello World done!\n");
}

int
main (void)
{
//	int count;

	/* Initialize GPIO (sets up clock) */
	GPIOInit ();

	/* reset PN532 */
	GPIOSetDir (PN532_RESET_PORT, PN532_RESET_PIN, 1);
	GPIOSetValue (PN532_RESET_PORT, PN532_RESET_PIN, 0);

	/* Set LED port pin to output */
	GPIOSetDir (LED_PORT, LED_BIT, 1);
	GPIOSetValue (LED_PORT, LED_BIT, LED_OFF);

	/* UART setup */
	UARTInit (115200, 0);

	/* CDC USB Initialization */
	usb_init ();

	/* Init Power Management Routines */
	pmu_init ();

	/* init PN532 RFID interface */
	pn532_init ();

	/* run RFID loop */
	buffer_get[0] = 0x01;

	/* show LED to signal initialization */
	GPIOSetValue (LED_PORT, LED_BIT, LED_ON);
	while (1)
	{
#if 0
		if ((count = usb_get (&buffer_get[1], PN532_FIFO_SIZE)) > 0)
		{
			debug_printf ("HOST->PN532: %u bytes\n", count);
			spi_txrx (SPI_CS_PN532, &buffer_get, count + 1, NULL, 0);
		}
#endif
	}
	return 0;
}
