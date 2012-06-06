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
#include "rfid.h"
#include "usbserial.h"

#define PN532_FIFO_SIZE 64

static uint8_t buffer_get[PN532_FIFO_SIZE + 1];
static const uint8_t hsu_wakeup[] = { 0x55, 0x55, 0x00, 0x00, 0x00 };

int
main (void)
{
	int i, t, count;
	uint8_t data, *p;

	/* Initialize GPIO (sets up clock) */
	GPIOInit ();

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
	rfid_init ();

	debug_printf ("OpenPCD2 v" PROGRAM_VERSION "\n");

	/* show LED to signal initialization */
	GPIOSetValue (LED_PORT, LED_BIT, LED_ON);
	pmu_wait_ms (500);

	/* get firmware version */
	data = PN532_CMD_GetFirmwareVersion;
	while (1)
	{
		if (((i = rfid_write (&data, sizeof (data))) == 0) &&
			((i = rfid_read (buffer_get, sizeof (buffer_get))) > 0))
			break;

		debug ("res=%i\n", i);
		pmu_wait_ms (490);
		GPIOSetValue (LED_PORT, LED_BIT, LED_ON);
		pmu_wait_ms (10);
		GPIOSetValue (LED_PORT, LED_BIT, LED_OFF);
	}

	if (buffer_get[1] == 0x32)
		debug_printf ("PN532 firmware version: v%i.%i\n",
					  buffer_get[2], buffer_get[3]);
	else
		debug ("Unknown firmware version\n");

	/* enable debug output */
	debug ("\nenabling debug output...\n");
	rfid_write_register (0x6328, 0xFC);
	/* select test bus signal */
	rfid_write_register (0x6321, 6);
	/* select test bus type */
	rfid_write_register (0x6322, 0x07);

	/* run RFID loop */
	buffer_get[0] = 0x01;
	t = 0;
	while (1)
	{
		if (!GPIOGetValue (PN532_IRQ_PORT, PN532_IRQ_PIN))
		{
			GPIOSetValue (LED_PORT, LED_BIT, (t++) & 1);

			debug ("RX: ");

			data = 0x03;

			spi_txrx (SPI_CS_PN532 | SPI_CS_MODE_SKIP_CS_DEASSERT, &data,
					  sizeof (data), NULL, 0);

			i = 0;
			while (!GPIOGetValue (PN532_IRQ_PORT, PN532_IRQ_PIN))
			{
				spi_txrx ((SPI_CS_PN532 ^ SPI_CS_MODE_SKIP_TX) |
						  SPI_CS_MODE_SKIP_CS_ASSERT |
						  SPI_CS_MODE_SKIP_CS_DEASSERT, NULL, 0,
						  &data, sizeof (data));

				usb_putchar (data);
				debug (" %02X", data);
				i++;
			}
			spi_txrx (SPI_CS_PN532 | SPI_CS_MODE_SKIP_CS_ASSERT, NULL, 0,
					  NULL, 0);

			if (i)
			{
				usb_putchar (0x00);
				usb_flush ();
				debug ("-00");
			}
			debug ("\n");

		}

		if ((count = usb_get (&buffer_get[1], PN532_FIFO_SIZE)) > 0)
		{
			GPIOSetValue (LED_PORT, LED_BIT, (t++) & 1);

			p = &buffer_get[1];

			/* erase FIFO after seeing HSU wakeup */
			if ((count == sizeof (hsu_wakeup))
				&& (memcmp (p, &hsu_wakeup, sizeof (hsu_wakeup)) == 0))
			{
				count = PN532_FIFO_SIZE;
				memset (&buffer_get[1], 0, PN532_FIFO_SIZE);
			}
			else
			{
				debug ("TX: ");
				for (i = 0; i < count; i++)
					debug ("%c%02X", i == 6 ? '*' : ' ', *p++);
				debug ("\n");
			}

			spi_txrx (SPI_CS_PN532, &buffer_get, count + 1, NULL, 0);
		}

	}
	return 0;
}
