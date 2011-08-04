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
#include "pmu.h"

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
  debug_printf ("CMD: '%s'\n", command);
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
#endif /*ENABLE_USB_FULLFEATURED */


static void
rfid_hexdump (const void *buffer, int size)
{
  int i;
  const unsigned char *p = (unsigned char *) buffer;

  for (i = 0; i < size; i++)
    {
      if (i && ((i & 3) == 0))
	debug_printf (" ");
      debug_printf (" %02X", *p++);
    }
  debug_printf (" [size=%02i]\n", size);
}

#if 0
static void
halt_error (const char *message, int res)
{
  while (1)
    {
      debug_printf ("\nError: %s [res=%i]\n", message, res);

      pmu_wait_ms (500);
      GPIOSetValue (LED_PORT, LED_BIT, LED_ON);
      pmu_wait_ms (500);
      GPIOSetValue (LED_PORT, LED_BIT, LED_OFF);
    }
}
#endif

static void
loop_rfid (void)
{
  int res;
  static unsigned char data[80];

  /* release reset line after 400ms */
  pmu_wait_ms (400);
  rfid_reset (1);
  /* wait for PN532 to boot */
  pmu_wait_ms (100);

  /* fully initialized */
  GPIOSetValue (LED_PORT, LED_BIT, LED_ON);

  /* read firmware revision */
  debug_printf ("\nreading firmware version...\n");
  data[0] = PN532_CMD_GetFirmwareVersion;
  while ((res = rfid_execute (&data, 1, sizeof (data))) < 0)
  {
    debug_printf ("Reading Firmware Version Error [%i]\n", res);
    pmu_wait_ms (450);
    GPIOSetValue (LED_PORT, LED_BIT, LED_ON);
    pmu_wait_ms (10);
    GPIOSetValue (LED_PORT, LED_BIT, LED_OFF);
  }

  debug_printf ("PN532 Firmware Version: ");
  if (data[1] != 0x32)
    rfid_hexdump (&data[1], data[0]);
  else
    debug_printf ("v%i.%i\n", data[2], data[3]);

  /* enable debug output */
  debug_printf ("\nenabling debug output...\n");
  rfid_write_register (0x6328, 0xFC);
  // select test bus signal
  rfid_write_register (0x6321, 6);
  // select test bus type
  rfid_write_register (0x6322, 0x07);

  /* enable debug output */
  GPIOSetValue (LED_PORT, LED_BIT, LED_ON);
  while (1)
    {
      /* wait 10ms */
      pmu_wait_ms (10);

      /* detect cards in field */
      data[0] = PN532_CMD_InListPassiveTarget;
      data[1] = 0x01;		/* MaxTg - maximum cards    */
      data[2] = 0x00;		/* BrTy - 106 kbps type A   */
      if (((res = rfid_execute (&data, 3, sizeof (data))) >= 11) && (data[1]
								     == 0x01)
	  && (data[2] == 0x01))
	{
	  GPIOSetValue (LED_PORT, LED_BIT, LED_ON);
	  pmu_wait_ms (50);
	  GPIOSetValue (LED_PORT, LED_BIT, LED_OFF);

	  debug_printf ("card id: ");
	  rfid_hexdump (&data[7], data[6]);
	}
      else
	{
	  GPIOSetValue (LED_PORT, LED_BIT, LED_ON);
	  if (res != -8)
	    debug_printf ("PN532 error res=%i\n", res);
	}

      /* turning field off */
      data[0] = PN532_CMD_RFConfiguration;
      data[1] = 0x01;		/* CfgItem = 0x01           */
      data[2] = 0x00;		/* RF Field = off           */
      rfid_execute (&data, 3, sizeof (data));
    }
}

int
main (void)
{
  /* Initialize GPIO (sets up clock) */
  GPIOInit ();

  /* Set LED port pin to output */
  GPIOSetDir (LED_PORT, LED_BIT, 1);
  GPIOSetValue (LED_PORT, LED_BIT, LED_OFF);

  /* Init RFID */
  rfid_init ();

  /* Init Power Management Routines */
  pmu_init ();

  /* UART setup */
  UARTInit (115200, 0);

  /* CDC Initialization */
#ifdef  ENABLE_USB_FULLFEATURED
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
#endif /*ENABLE_USB_FULLFEATURED */

  /* RUN RFID loop */
  loop_rfid ();

  return 0;
}
