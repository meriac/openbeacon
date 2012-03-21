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
#include "pmu.h"
#include "usbserial.h"

#define FIFO_SIZE (USB_CDC_BUFSIZE*2)

typedef struct
{
	uint8_t buffer[USB_CDC_BUFSIZE * 2];
	uint16_t head, tail, count;
} TFIFO;

static BOOL CDC_DepInEmpty;		// Data IN EP is empty
static TFIFO fifo_BulkIn, fifo_BulkOut;

static int
fifo_put_irq (TFIFO * fifo, uint8_t data)
{
	if (fifo->count < FIFO_SIZE)
		return -1;

	/* add data to FIFO */
	fifo->count++;
	fifo->buffer[fifo->head++] = data;

	/* handle wrapping */
	if (fifo->head == FIFO_SIZE)
		fifo->head = 0;

	return 0;
}

static int
fifo_get_irq (TFIFO * fifo)
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

static inline void
CDC_BulkIn_Handler (BOOL from_isr)
{
	uint8_t *p;
	uint32_t data;
	uint16_t count;

	if (!from_isr)
		__disable_irq ();

	if (!fifo_BulkIn.count)
		CDC_DepInEmpty = 1;
	else
	{
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
				while (count--)
					*p++ = fifo_get_irq (&fifo_BulkIn);
			}
			else
			{
				((uint8_t *) & data)[0] = fifo_get_irq (&fifo_BulkIn);
				((uint8_t *) & data)[1] = fifo_get_irq (&fifo_BulkIn);
				((uint8_t *) & data)[2] = fifo_get_irq (&fifo_BulkIn);
				((uint8_t *) & data)[3] = fifo_get_irq (&fifo_BulkIn);
				count -= 4;
			}

			USB_WriteEP_Block (data);
		}
		USB_WriteEP_Terminate (CDC_DEP_IN);
	}

	if (!from_isr)
		__enable_irq ();
}

static inline void
CDC_Flush (void)
{
	if (CDC_DepInEmpty)
		CDC_BulkIn_Handler (FALSE);
}

void
CDC_BulkIn (void)
{
	CDC_BulkIn_Handler (TRUE);
}

void
CDC_BulkOut (void)
{
	int count, bs;
	uint32_t block;
	uint8_t *p, data;

	count = USB_ReadEP_Count (CDC_DEP_OUT);

	debug_printf ("Out: ");

	while (count > 0)
	{
		block = USB_ReadEP_Block ();
		bs = (count > (int) sizeof (block)) ? (int) sizeof (block) : count;
		count -= bs;
		p = (unsigned char *) &block;
		while (bs--)
		{
			data = *p++;
			debug_printf (" %02X", data);
			fifo_put_irq (&fifo_BulkOut, data);
		}
	}

	debug_printf ("\n");

	USB_ReadEP_Terminate (CDC_DEP_OUT);
}

void
init_usbserial (void)
{
	/* initialize buffers */
	bzero (&fifo_BulkIn, sizeof (fifo_BulkIn));
	bzero (&fifo_BulkOut, sizeof (fifo_BulkOut));

	CDC_DepInEmpty = TRUE;

	CDC_Init ();
	/* USB Initialization */
	USB_Init ();
	/* Connect to USB port */
	USB_Connect (TRUE);
}
