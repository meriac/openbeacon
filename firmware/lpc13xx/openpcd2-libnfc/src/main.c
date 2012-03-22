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

uint8_t buffer_get[PN532_FIFO_SIZE + 1];

#if 0
static uint8_t
fifo_status (void)
{
	uint8_t res;
	static const uint8_t pkt = 0x2;

	spi_txrx (SPI_CS_PN532, &pkt, sizeof (pkt), &res, sizeof (res));

	return res;
}
#endif

int
main (void)
{
	int count;
	uint8_t data, *p;

	/* Initialize GPIO (sets up clock) */
	GPIOInit ();

	/* reset PN532 */
	GPIOSetDir (PN532_RESET_PORT, PN532_RESET_PIN, 1);
	GPIOSetValue (PN532_RESET_PORT, PN532_RESET_PIN, 0);
	/* initialize PN532 IRQ line */
	GPIOSetDir (PN532_IRQ_PORT, PN532_IRQ_PIN, 0);

	/* Set LED port pin to output */
	GPIOSetDir (LED_PORT, LED_BIT, 1);
	GPIOSetValue (LED_PORT, LED_BIT, LED_OFF);

	/* UART setup */
	UARTInit (115200, 0);

	/* CDC USB Initialization */
	usb_init ();

	/* Init Power Management Routines */
	pmu_init ();

	/* init RFID SPI interface */
	spi_init ();
	spi_init_pin (SPI_CS_PN532);

	/* release reset line after 400ms */
	pmu_wait_ms (400);
	GPIOSetValue (PN532_RESET_PORT, PN532_RESET_PIN, 1);

	/* wait for PN532 to boot */
	pmu_wait_ms (100);

	/* run RFID loop */
	buffer_get[0] = 0x01;

	/* show LED to signal initialization */
	GPIOSetValue (LED_PORT, LED_BIT, LED_ON);
	while (1)
	{
		if ((count = usb_get (&buffer_get[1], PN532_FIFO_SIZE)) > 0)
		{
			spi_txrx (SPI_CS_PN532, &buffer_get, count + 1, NULL, 0);

			p = &buffer_get[1];

			debug_printf ("TX: ");
			while (count--)
				debug_printf (" %02X", *p++);
			debug_printf ("\n");
		}

		if (!GPIOGetValue (PN532_IRQ_PORT, PN532_IRQ_PIN))
		{
			data = 0x03;
			spi_txrx (SPI_CS_PN532 | SPI_CS_MODE_SKIP_CS_DEASSERT, &data,
					  sizeof (data), NULL, 0);

			debug_printf ("RX: ");
			while (!GPIOGetValue (PN532_IRQ_PORT, PN532_IRQ_PIN))
			{
				spi_txrx ((SPI_CS_PN532 ^ SPI_CS_MODE_SKIP_TX) |
						  SPI_CS_MODE_SKIP_CS_ASSERT |
						  SPI_CS_MODE_SKIP_CS_DEASSERT, NULL, 0,
						  &data, sizeof (data));

				usb_putchar (data);

				debug_printf (" %02X", data);
			}

			usb_putchar (0x00);
			debug_printf (" 00\n");

			GPIOSetDir (PN532_CS_PORT, PN532_CS_PIN, 1);

			usb_flush ();
		}
	}
	return 0;
}
