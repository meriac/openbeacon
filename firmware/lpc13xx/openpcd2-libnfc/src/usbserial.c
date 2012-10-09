/***************************************************************
 *
 * OpenBeacon.org - application specific USB functionality
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
#include "usbserial.h"

#define FIFO_SIZE (USB_CDC_BUFSIZE * 2)

typedef struct
{
	uint8_t buffer[FIFO_SIZE];
	uint16_t head, tail, count;
} TFIFO;

BOOL CDC_DepInEmpty;
TFIFO fifo_BulkIn, fifo_BulkOut;

int
usb_putchar_irq (TFIFO * fifo, uint8_t data)
{
	if (fifo->count >= FIFO_SIZE)
		return -1;

	/* add data to FIFO */
	fifo->count++;
	fifo->buffer[fifo->head++] = data;

	/* handle wrapping */
	if (fifo->head == FIFO_SIZE)
		fifo->head = 0;

	return 0;
}

int
usb_getchar_irq (TFIFO * fifo)
{
	int res;

	if (!fifo->count)
		return -1;

	/* get data from FIFO */
	fifo->count--;
	res = fifo->buffer[fifo->tail++];

	/* handle wrapping */
	if (fifo->tail == FIFO_SIZE)
		fifo->tail = 0;

	return res;
}

int
usb_getchar (void)
{
	int res;

	__disable_irq ();
	res = usb_getchar_irq (&fifo_BulkOut);
	__enable_irq ();

	return res;
}

int
usb_putchar (uint8_t data)
{
	int res;

	__disable_irq ();
	/* if USB FIFO is full - flush */
	if (fifo_BulkIn.count >= USB_CDC_BUFSIZE)
		CDC_BulkIn ();
	/* store new data in FIFO */
	res = usb_putchar_irq (&fifo_BulkIn, data);
	__enable_irq ();

	return res;
}

void
CDC_BulkIn (void)
{
	uint8_t *p;
	uint32_t data;
	uint16_t count;

	if (!fifo_BulkIn.count)
		return;

	if (fifo_BulkIn.count > USB_CDC_BUFSIZE)
		count = USB_CDC_BUFSIZE;
	else
		count = fifo_BulkIn.count;

	USB_WriteEP_Count (CDC_DEP_IN, count);

	while (count > 0)
	{
		if (count < (int) sizeof (data))
		{
			data = 0;
			p = (uint8_t *) & data;
			while (count)
			{
				count--;
				*p++ = usb_getchar_irq (&fifo_BulkIn);
			}
		}
		else
		{
			((uint8_t *) & data)[0] = usb_getchar_irq (&fifo_BulkIn);
			((uint8_t *) & data)[1] = usb_getchar_irq (&fifo_BulkIn);
			((uint8_t *) & data)[2] = usb_getchar_irq (&fifo_BulkIn);
			((uint8_t *) & data)[3] = usb_getchar_irq (&fifo_BulkIn);
			count -= 4;
		}
		USB_WriteEP_Block (data);
	}

	USB_WriteEP_Terminate (CDC_DEP_IN);
}

void
usb_flush (void)
{
	__disable_irq ();
	CDC_BulkIn ();
	__enable_irq ();
}

void
CDC_BulkOut (void)
{
	int count, bs;
	uint32_t block;
	uint8_t *p;

	count = USB_ReadEP_Count (CDC_DEP_OUT);

	while (count > 0)
	{
		block = USB_ReadEP_Block ();
		bs = (count > (int) sizeof (block)) ? (int) sizeof (block) : count;
		count -= bs;
		p = (unsigned char *) &block;
		while (bs--)
			usb_putchar_irq (&fifo_BulkOut, *p++);
	}

	USB_ReadEP_Terminate (CDC_DEP_OUT);
}

void
usb_init (void)
{
	/* initialize buffers */
	bzero (&fifo_BulkIn, sizeof (fifo_BulkIn));
	bzero (&fifo_BulkOut, sizeof (fifo_BulkOut));

	CDC_Init ();
	/* USB Initialization */
	USB_Init ();
	/* Connect to USB port */
	USB_Connect (TRUE);
}
