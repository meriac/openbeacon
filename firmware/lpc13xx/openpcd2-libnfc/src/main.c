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
#define PN532_MAX_PAYLAOADSIZE 264
#define PN532_MAX_PACKET_SIZE (PN532_MAX_PAYLAOADSIZE+11)
typedef enum
{
	STATE_IDLE = 0,
	STATE_PREFIX = -1,
	STATE_PREFIX_EXT = -2,
	STATE_HEADER = -3,
	STATE_WAKEUP = -4,
	STATE_FIFOFLUSH = -5,
	STATE_POSTAMBLE = -6,
	STATE_PAYLOAD = -7,
	STATE_FULLPACKET = -8
} PN532_State;

typedef struct {
	uint32_t last_seen;
	uint16_t reserved;
	uint16_t pos;
	uint16_t expected;
	uint8_t data_prev;
	uint8_t wakeup;
	uint8_t crc;
	PN532_State state;
	uint8_t data[PN532_MAX_PACKET_SIZE + 1];
} PN532_Packet;

static uint8_t buffer_get[PN532_MAX_PACKET_SIZE + 1];
static PN532_Packet buffer_put;

static void
packet_reset (PN532_Packet* pkt, uint8_t reserved)
{
	memset(pkt, 0, sizeof(pkt));
	pkt->reserved = reserved;
	pkt->data_prev = 0x01;
}

static int
packet_put (PN532_Packet* pkt, uint8_t data)
{
	PN532_State res;
	uint8_t len, lcs;
	const uint8_t prefix[] = { 0x00, 0x00, 0xFF };

	debug ("RX: <IN> 0x%02X [pos=%02u,state=%i, expected=%02i]\n", data, pkt->pos, pkt->state, pkt->expected);

	res = pkt->state;

	switch (pkt->state)
	{
		case STATE_FULLPACKET:
		{
			packet_reset (pkt, pkt->reserved );
			/* intentionally no 'break;' */
		}

		case STATE_WAKEUP:
		{
			res = STATE_IDLE;
			/* intentionally no 'break;' */
		}

		case STATE_IDLE:
		{
			/* scan for 0x00+0xFF prefix */
			if (data==0xFF && pkt->data_prev==0x00)
			{
				memcpy (&pkt->data[pkt->reserved], prefix, sizeof(prefix));
				/* add size of reserved+prefix to packet pos */
				pkt->pos = pkt->reserved+sizeof(prefix);
				/* expect at least a short frame */
				pkt->expected = pkt->pos + 3;
				/* switch to prefix reception mode */
				res = STATE_PREFIX;
				break;
			}

			/* scan for HSU wakeup */
			if (data==0x55 && pkt->data_prev==0x55)
				/* wait for three times 0x00 */
				pkt->wakeup = 3;
			else
				if (pkt->wakeup)
				{
					if(data)
						pkt->wakeup = 0;
					else
					{
						pkt->wakeup--;
						if(!pkt->wakeup)
						{
							res = STATE_WAKEUP;
							break;
						}
					}
				}

			break;
		}

		case STATE_PREFIX:
		{
			pkt->data[pkt->pos++] = data;
			if(pkt->pos>=pkt->expected)
			{
				hex_dump (pkt->data, 0, pkt->pos);

				lcs = pkt->data[pkt->pos-2];
				len = pkt->data[pkt->pos-3];

				debug ("IR: valid packet with LEN=0x%02X LCS=0x%02X\n",len,lcs);

				/* detected extended frame */
				if (len == 0xFF && lcs == 0xFF)
				{
					debug ("IR: extended frame\n");
					/* expect three more bytes for extended frame */
					pkt->expected+=3;
					res = STATE_PREFIX_EXT;
					break;
				}

				/* detected ACK frame */
				if (len == 0xFF && lcs == 0x00)
				{
					debug ("IR: NACK frame\n");
					res = STATE_POSTAMBLE;
					break;
				}

				/* detected NACK frame */
				if (len == 0x00 && lcs == 0xFF)
				{
					debug ("IR: ACK frame\n");
					res = STATE_POSTAMBLE;
					break;
				}

				if (len == 0x01 && lcs == 0xFF)
				{
					pkt->expected+=len;
					debug ("IR: ERROR frame [error=0x%02X]\n",pkt->data[pkt->pos-1]);
					pkt->crc=pkt->data[pkt->pos-1];
					res = STATE_PAYLOAD;
					break;
				}

				/* if valid short packet */
				if ( ((uint8_t)(len+lcs)) == 0 )
				{
					pkt->expected+=len;
					debug ("IR: short frame [%u payload, %u total]\n", len, pkt->expected);

					/*detect oversized packets */
					if(pkt->expected > PN532_MAX_PACKET_SIZE)
					{
						debug ("IR: oversized frame\n");
						packet_reset (pkt, pkt->reserved );
						res = STATE_IDLE;
					}
					else
					{
						/* check for TFI */
						if (pkt->data[pkt->pos-1] == 0xD4)
						{
							/* maintain CRC including TFI */
							pkt->crc=0xD4;
							res = STATE_PAYLOAD;
						}
						else
						{
							debug ("IR: wrong TFI [0x%02X]\n", pkt->data[pkt->pos-1]);
							packet_reset (pkt, pkt->reserved );
							res = STATE_IDLE;
						}
					}

					break;
				}
			}
			break;
		}

		case STATE_PREFIX_EXT:
		{
			/* TODO: add extended frame support */
			debug ("IR: extended frame is not yet supported\n");
			packet_reset (pkt, pkt->reserved );
			res = STATE_IDLE;
			break;
		}

		case STATE_PAYLOAD:
		{
			pkt->data[pkt->pos++] = data;
			pkt->crc+=data;

			if(pkt->pos>=pkt->expected)
			{
				if(pkt->crc)
				{
					debug ("IR: packet CRC error [0x%02X]\n", pkt->crc);
					packet_reset (pkt, pkt->reserved );
					res = STATE_IDLE;
				}
				else
					res = STATE_POSTAMBLE;
			}
			break;
		}

		case STATE_POSTAMBLE:
		{
			if(data == 0x00)
			{
				debug ("IR: full packet - CRC OK\n");

				/* return packet size */
				pkt->data[pkt->pos++] = data;
				res = pkt->pos;
			}
			else
			{
				debug ("IR: packet postamble error [0x%02X]\n", data);
				packet_reset (pkt, pkt->reserved );
				res = STATE_IDLE;
			}
			break;
		}

		default:
		{
			debug ("IR: unknown state!!!\n");
			packet_reset (pkt, pkt->reserved );
			res = STATE_IDLE;
		}
	}

	pkt->data_prev = data;
	pkt->state = (res>0) ? STATE_FULLPACKET:res;

	debug ("RX: <OT> 0x%02X [pos=%02u,state=%i, expected=%02i]\n", data, pkt->pos, pkt->state, pkt->expected);

	return res;
}

int
main (void)
{
	int i, t, count, res;
	uint8_t data;

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
			((i = rfid_read (buffer_get, PN532_FIFO_SIZE))) > 0)
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

	/* run RFID loop */
	buffer_get[0] = 0x01;
	t = 0;

	packet_reset ( &buffer_put, 1 );
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

		if ((res = usb_getchar ()) >= 0)
		{
			if((count = packet_put(&buffer_put, (uint8_t)res))>0)
			{
				GPIOSetValue (LED_PORT, LED_BIT, (t++) & 1);

#ifdef  DEBUG
				uint8_t *p = &buffer_put.data[1];
				debug ("IT: ");
				for (i = 0; i < (count-1); i++)
					debug ("%c%02X", i == 6 ? '*' : ' ', *p++);
				debug ("\n");
#endif/*DEBUG*/

				buffer_put.data[0] = 0x01;
				spi_txrx (SPI_CS_PN532, buffer_put.data, count, NULL, 0);
			}
			else
				switch(count)
				{
					case STATE_WAKEUP :
						/* reset PN532 */
						GPIOSetValue (PN532_RESET_PORT, PN532_RESET_PIN, 0);
						pmu_wait_ms (100);
						GPIOSetValue (PN532_RESET_PORT, PN532_RESET_PIN, 1);
						pmu_wait_ms (400);
						count = 0;
						break;
					case STATE_FIFOFLUSH :
						/* flush PN532 buffers */
						buffer_put.data[0] = 0x01;
						memset (&buffer_put.data[1], 0, PN532_FIFO_SIZE);
						spi_txrx (SPI_CS_PN532, buffer_put.data, PN532_FIFO_SIZE+1, NULL, 0);
						break;
				}
		}
	}
	return 0;
}
