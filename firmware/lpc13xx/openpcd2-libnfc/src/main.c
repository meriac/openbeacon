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

static uint8_t buffer_get[PN532_FIFO_SIZE + 1];
static const uint8_t hsu_wakeup[] = { 0x55, 0x55, 0x00, 0x00, 0x00 };

#ifdef  DEBUG
#define debug(args...) debug_printf(args)
#else /* no DEBUG enable - remove debug code */
#define debug(...) {}
#endif /*DEBUG*/
static uint8_t
rfid_rx (void)
{
	uint8_t byte;
	spi_txrx ((SPI_CS_PN532 ^ SPI_CS_MODE_SKIP_TX) |
			  SPI_CS_MODE_SKIP_CS_ASSERT |
			  SPI_CS_MODE_SKIP_CS_DEASSERT, NULL, 0, &byte, sizeof (byte));

	return byte;
}

static void
rfid_tx (uint8_t byte)
{
	spi_txrx ((SPI_CS_PN532 ^ SPI_CS_MODE_SKIP_TX) |
			  SPI_CS_MODE_SKIP_CS_ASSERT |
			  SPI_CS_MODE_SKIP_CS_DEASSERT, &byte, sizeof (byte), NULL, 0);
}

void
rfid_send (const void *data, int len)
{
	static const uint8_t preamble[] = { 0x01, 0x00, 0x00, 0xFF };
	const uint8_t *p = preamble;
	uint8_t tfi = 0xD4, c;

	spi_txrx (SPI_CS_PN532 | SPI_CS_MODE_SKIP_CS_DEASSERT, &preamble,
			  sizeof (preamble), NULL, 0);

	rfid_tx (len + 1);															/* LEN */
	rfid_tx (0x100 - (len + 1));												/* LCS */
	rfid_tx (tfi);																/* TFI */

	/* PDn */
	p = (const uint8_t *) data;
	while (len--)
	{
		c = *p++;
		rfid_tx (c);
		tfi += c;
	}
	rfid_tx (0x100 - tfi);														/* DCS */
	rfid_rx ();																	/* Postamble */
}

int
main (void)
{
	int i, count;
	uint8_t t, data, *p;

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

	debug_printf ("OpenPCD2 v" PROGRAM_VERSION "\n");

	/* show LED to signal initialization */
	GPIOSetValue (LED_PORT, LED_BIT, LED_ON);
	pmu_wait_ms (500);

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
