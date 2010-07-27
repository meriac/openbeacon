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
#include "hid.h"
#include "rfid.h"

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

static int
rfid_execute (void *data, unsigned int isize, unsigned int osize)
{
  int res;
  if (rfid_write (data, isize))
    {
      debug_printf ("getting result\n");
      res = rfid_read (data, osize);
      if (res > 0)
	rfid_hexdump (data, res);
      else
	debug_printf ("error: res=%i\n", res);
    }
  else
    {
      debug_printf ("->NACK!\n");
      res = -1;
    }
  return res;
}

int
main (void)
{
  unsigned char data[80];

  /* UART setup */
  UARTInit (115200);

  /* GPIO setup */
  GPIOInit ();
  GPIOSetDir (LED_PORT, LED_BIT, 1);
  GPIOSetDir (0, 6, 1);		/* fake USB connect */

  /* Init RFID */
  rfid_init ();

  /* Init USB HID interface */
  hid_init ();

  /* Hello World */
  debug_printf ("Hello RFID!\n");

  /* main loop */
  while (1)
    {
      /* read firmware revision */
      debug_printf ("\nreading firmware version...\n");
      data[0] = 0x02;		/* cmd: GetFirmWareVersion  */
      rfid_execute (&data, 1, sizeof (data));

      /* detect cards in field */
      debug_printf ("\nchecking for cards...\n");
      data[0] = 0x4A;		/* cmd: InListPassiveTarget */
      data[1] = 0x02;		/* MaxTg - maximum cards    */
      data[2] = 0x00;		/* BrTy - 106 kbps type A   */

      /* fake blinking with USB-connect */
      GPIOSetValue (0, 6, 1);
      rfid_execute (&data, 3, sizeof (data));
      GPIOSetValue (0, 6, 0);

      /* turning field off */
      debug_printf ("\nturning field off again...\n");
      data[0] = 0x32;		/* cmd: RFConfiguration     */
      data[1] = 0x01;		/* CfgItem = 0x01           */
      data[2] = 0x00;		/* RF Field = off           */
      rfid_execute (&data, 3, sizeof (data));
    }
}
