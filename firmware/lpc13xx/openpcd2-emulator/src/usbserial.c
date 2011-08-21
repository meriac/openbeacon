/***************************************************************
 *
 * OpenBeacon.org - application specific USB functionality
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
#include "main.h"
#include "pmu.h"
#include "usbserial.h"

#ifdef ENABLE_USB_FULLFEATURED
static BOOL CDC_DepInEmpty;	// Data IN EP is empty
static unsigned char fifo_out[USB_CDC_BUFSIZE], fifo_in[128];
static int fifo_out_count, fifo_in_count;

static inline void
CDC_BulkIn_Handler (BOOL from_isr)
{
  uint32_t data, *p;

  if (!from_isr)
    __disable_irq ();

  if (!fifo_out_count)
    CDC_DepInEmpty = 1;
  else
    {
      USB_WriteEP_Count (CDC_DEP_IN, fifo_out_count);
      p = (uint32_t *) & fifo_out;
      while (fifo_out_count > 0)
	{
	  if (fifo_out_count >= (int) sizeof (data))
	    {
	      fifo_out_count -= sizeof (data);
	      data = *p++;
	    }
	  else
	    {
	      data = 0;
	      memcpy (&data, p, fifo_out_count);
	      fifo_out_count = 0;
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

static inline void
CDC_GetCommand (unsigned char *command)
{
  debug_printf ("Unknown command: '%s'\n", command);
  CDC_Flush ();
}

static inline void
CDC_GetChar (unsigned char data)
{
  if (fifo_in_count < (int) (sizeof (fifo_in) - 1))
    {
      if (data < ' ')
	{
	  /* add line termination */
	  fifo_in[fifo_in_count] = 0;
	  CDC_GetCommand (fifo_in);
	  fifo_in_count = 0;
	}
      else
	fifo_in[fifo_in_count++] = data;
    }
}

void
CDC_BulkOut (void)
{
  int count, bs;
  uint32_t data;
  unsigned char *p;

  count = USB_ReadEP_Count (CDC_DEP_OUT);

  while (count > 0)
    {
      data = USB_ReadEP_Block ();
      bs = (count > (int) sizeof (data)) ? (int) sizeof (data) : count;
      count -= bs;
      p = (unsigned char *) &data;
      while (bs--)
	CDC_GetChar (*p++);
    }

  USB_ReadEP_Terminate (CDC_DEP_OUT);
}

BOOL
default_putchar (uint8_t data)
{
  __disable_irq ();

  if (fifo_out_count < (int) sizeof (fifo_out))
    fifo_out[fifo_out_count++] = data;

  if (data == '\n')
    CDC_BulkIn_Handler (TRUE);

  __enable_irq ();

  return UARTSendChar (data);
}

void
init_usbserial (void)
{
  fifo_out_count = fifo_in_count = 0;
  CDC_DepInEmpty = TRUE;

  CDC_Init ();
  /* USB Initialization */
  USB_Init ();
  /* Connect to USB port */
  USB_Connect (TRUE);
  /* wait until USB is initialized */
  while (!USB_Configuration)
  {
      pmu_wait_ms (90);
      GPIOSetValue (LED_PORT, LED_BIT, LED_ON);
      pmu_wait_ms (10);
      GPIOSetValue (LED_PORT, LED_BIT, LED_OFF);
  }
}
#endif /*ENABLE_USB_FULLFEATURE*/
