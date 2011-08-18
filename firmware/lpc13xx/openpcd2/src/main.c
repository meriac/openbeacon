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
#include "pmu.h"
#include "usbserial.h"

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

  /* CDC USB Initialization */
  init_usbserial ();

  /* RUN RFID loop */
  loop_rfid ();

  return 0;
}
