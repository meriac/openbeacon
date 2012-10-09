/***************************************************************
 *
 * OpenBeacon.org - PN532 routines for LPC13xx based OpenPCD2
 *
 * Copyright 2010-2012 Milosch Meriac <meriac@openbeacon.de>
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

#ifdef  ENABLE_PN532_RFID

#include "pn532.h"
#include "rfid.h"

void
rfid_reset (unsigned char reset)
{
	GPIOSetValue (PN532_RESET_PORT, PN532_RESET_PIN, reset ? 1 : 0);
}

static void
rfid_cs (unsigned char cs)
{
	GPIOSetValue (PN532_CS_PORT, PN532_CS_PIN, cs ? 1 : 0);
}

static void
rfid_tx (unsigned char data)
{
	spi_txrx ((SPI_CS_PN532 ^ SPI_CS_MODE_SKIP_TX) |
			  SPI_CS_MODE_SKIP_CS_ASSERT |
			  SPI_CS_MODE_SKIP_CS_DEASSERT, &data, sizeof (data), NULL, 0);
}

static unsigned char
rfid_rx (void)
{
	unsigned char data;
	spi_txrx ((SPI_CS_PN532 ^ SPI_CS_MODE_SKIP_TX) |
			  SPI_CS_MODE_SKIP_CS_ASSERT |
			  SPI_CS_MODE_SKIP_CS_DEASSERT, NULL, 0, &data, sizeof (data));


	return data;
}

int
rfid_read (void *data, unsigned char size)
{
	int res;
	unsigned char *p, c, pkt_size, crc, prev, t;

	/* wait 100ms max till PN532 response is ready */
	t = 0;
	while (GPIOGetValue (PN532_IRQ_PORT, PN532_IRQ_PIN))
	{
		if (t++ > 10)
			return -8;
		pmu_wait_ms (10);
	}

	debug ("RI: ");

	/* enable chip select */
	rfid_cs (0);

	/* read from FIFO command */
	rfid_tx (0x03);

	/* default result */
	res = -9;

	/* find preamble */
	t = 0;
	prev = rfid_rx ();
	while ((!(((c = rfid_rx ()) == 0xFF) && (prev == 0x00)))
		   && (t < PN532_FIFO_SIZE))
	{
		prev = c;
		t++;
	}

	if (t >= PN532_FIFO_SIZE)
		res = -3;
	else
	{
		/* read packet size */
		pkt_size = rfid_rx ();

		/* special treatment for NACK and ACK */
		if ((pkt_size == 0x00) || (pkt_size == 0xFF))
		{
			/* verify if second length byte is inverted */
			if (rfid_rx () != (unsigned char) (~pkt_size))
				res = -2;
			else
			{
				/* eat Postamble */
				rfid_rx ();
				/* -1 for NACK, 0 for ACK */
				res = pkt_size ? -1 : 0;
			}
		}
		else
		{
			/* verify packet size against LCS */
			if (((pkt_size + rfid_rx ()) & 0xFF) != 0)
				res = -4;
			else
			{
				/* remove TFI from packet size */
				pkt_size--;
				/* verify if packet fits into buffer */
				if (pkt_size > size)
					res = -5;
				else
				{
					/* remember actual packet size */
					size = pkt_size;
					/* verify TFI */
					if ((crc = rfid_rx ()) != 0xD5)
						res = -6;
					else
					{
						/* read packet */
						p = (unsigned char *) data;
						while (pkt_size--)
						{
							/* read data */
							c = rfid_rx ();
							debug (" %02X", c);

							/* maintain crc */
							crc += c;
							/* save payload */
							if (p)
								*p++ = c;
						}

						/* add DCS to CRC */
						crc += rfid_rx ();
						/* verify CRC */
						if (crc)
							res = -7;
						else
						{
							/* eat Postamble */
							rfid_rx ();
							/* return actual size as result */
							res = size;
						}
					}
				}
			}
		}
	}
	rfid_cs (1);

	debug (" [%i]\n", res);

	/* everything fine */
	return res;
}

int
rfid_write (const void *data, int len)
{
	int i;
	static const unsigned char preamble[] = { 0x01, 0x00, 0x00, 0xFF };
	const unsigned char *p = preamble;
	unsigned char tfi = 0xD4, c;

	if (!data)
		len = 0xFF;

	debug ("TI: ");

	/* enable chip select */
	rfid_cs (0);

	p = preamble;																/* Praeamble */
	for (i = 0; i < (int) sizeof (preamble); i++)
		rfid_tx (*p++);
	rfid_tx (len + 1);															/* LEN */
	rfid_tx (0x100 - (len + 1));												/* LCS */
	rfid_tx (tfi);																/* TFI */
	/* PDn */
	p = (const unsigned char *) data;
	while (len--)
	{
		c = *p++;
		debug (" %02X", c);

		rfid_tx (c);
		tfi += c;
	}
	rfid_tx (0x100 - tfi);														/* DCS */
	rfid_rx ();																	/* Postamble */

	/* release chip select */
	rfid_cs (1);

	debug ("\n");

	/* check for ack */
	return rfid_read (NULL, 0);
}

int
rfid_execute (void *data, unsigned int isize, unsigned int osize)
{
	int res;

	if ((res = rfid_write (data, isize)) < 0)
		return res;
	else
		return rfid_read (data, osize);
}

int
rfid_write_register (unsigned short address, unsigned char data)
{
	unsigned char cmd[4];

	/* write register */
	cmd[0] = PN532_CMD_WriteRegister;
	/* high byte of address */
	cmd[1] = address >> 8;
	/* low byte of address */
	cmd[2] = address & 0xFF;
	/* data value */
	cmd[3] = data;

	return rfid_execute (&cmd, sizeof (cmd), sizeof (data));
}

int
rfid_read_register (unsigned short address)
{
	int res;
	unsigned char cmd[3];

	/* write register */
	cmd[0] = PN532_CMD_ReadRegister;
	/* high byte of address */
	cmd[1] = address >> 8;
	/* low byte of address */
	cmd[2] = address & 0xFF;

	if ((res = rfid_execute (&cmd, sizeof (cmd), sizeof (cmd))) > 1)
		return cmd[1];
	else
		return res;
}

int
rfid_mask_register (unsigned short address, unsigned char data,
					unsigned char mask)
{
	int res;

	if ((res = rfid_read_register (address)) < 0)
		return res;
	else
		return rfid_write_register (address,
									(((unsigned char) res) & (~mask)) | (data
																		 &
																		 mask));
}

void
rfid_init (void)
{
	/* Initialize RESET line */
	GPIOSetDir (PN532_RESET_PORT, PN532_RESET_PIN, 1);

	/* reset PN532 */
	GPIOSetValue (PN532_RESET_PORT, PN532_RESET_PIN, 0);

	/* initialize PN532 IRQ line */
	GPIOSetDir (PN532_IRQ_PORT, PN532_IRQ_PIN, 0);

	/* init RFID SPI interface */
	spi_init ();
	spi_init_pin (SPI_CS_PN532);

	/* release reset line after 400ms */
	pmu_wait_ms (400);
	GPIOSetValue (PN532_RESET_PORT, PN532_RESET_PIN, 1);

	/* wait for PN532 to boot */
	pmu_wait_ms (100);
}

#endif /*ENABLE_PN532_RFID */
