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
#include "pin.h"
#include "hid.h"
#include "spi.h"
#include "bluetooth.h"
#include "3d_acceleration.h"
#include "storage.h"
#include "nRF24L01.h"

static uint8_t hid_buffer[USB_HID_IN_REPORT_SIZE];

void
GetInReport (uint8_t src[], uint32_t length)
{
  (void) src;
  (void) length;

  if (length > USB_HID_IN_REPORT_SIZE)
    length = USB_HID_IN_REPORT_SIZE;

  memcpy (src, hid_buffer, length);
}

void
SetOutReport (uint8_t dst[], uint32_t length)
{
  (void) dst;
  (void) length;
}

/*
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
*/

void
main_menue (uint8_t cmd)
{
  /* ignore non-printable characters */
  if (cmd <= ' ')
    return;
  /* show key pressed */
  debug_printf ("%c\n", cmd);
  /* map lower case to upper case */
  if (cmd > 'a' && cmd < 'z')
    cmd -= ('a' - 'A');

  switch (cmd)
    {
    case '?':
    case 'H':
      debug_printf ("\n"
		    " *****************************************************\n"
		    " * OpenBeacon USB II - Bluetooth Console             *\n"
		    " * (C) 2010 Milosch Meriac <meriac@openbeacon.de>    *\n"
		    " *****************************************************\n"
		    " * H,?          - this help screen\n"
		    " * S            - SPI status\n"
		    " *****************************************************\n"
		    "\n");
      break;
    case 'S':
      debug_printf ("\n"
		    " *****************************************************\n"
		    " * OpenBeacon Status Information                     *\n"
		    " *****************************************************\n");
      spi_status ();
      acc_status ();
      storage_status ();
      debug_printf (" *****************************************************\n"
		    "\n");
      break;
    default:
      debug_printf ("Unknown command '%c' - please press 'H' for help \n",
		    cmd);
    }
  debug_printf ("\n# ");
}

int
main (void)
{
  int t, firstrun;
  volatile int i;

  /* Initialize GPIO (sets up clock) */
  GPIOInit ();
  /* initialize  pins */
  pin_init ();
  /* setup SPI chipselect pins */
  spi_init_pin (SPI_CS_NRF);

  /* blink as a sign of boot to detect crashes */
  for (t = 0; t < 10; t++)
    {
      pin_led (GPIO_LED0);
      for (i = 0; i < 100000; i++);
      pin_led (GPIO_LEDS_OFF);
      for (i = 0; i < 100000; i++);
    }

  /* Init USB HID interface */
  hid_init ();
  /* Init OpenBeacon nRF24L01 interface */
  nrf_init ();
  /* Init SPI */
  spi_init ();
  /* Init 3D acceleration sensor */
  acc_init ();
  /* Init Storage */
  storage_init ();
  /* Init Bluetooth */
  bt_init ();

  /* main loop */
  t = 0;
  firstrun = 1;
  while (1)
    {
      /* blink LED0 on every 32th run - FIXME later with sleep */
      if ((t++ & 0x1F) == 0)
	{
	  pin_led (GPIO_LED0);
	  for (i = 0; i < 100000; i++);
	  pin_led (GPIO_LEDS_OFF);
	}
      for (i = 0; i < 200000; i++);

      if (UARTCount)
	{
	  /* blink LED1 upon Bluetooth command */
	  pin_led (GPIO_LED1);

	  /* show help screen upon Bluetooth connect */
	  if (firstrun)
	    {
	      debug_printf ("press 'H' for help...\n# ");
	      firstrun = 0;
	    }
	  else
	    /* execute menue command with last character received */
	    main_menue (UARTBuffer[UARTCount - 1]);

	  /* LED1 off again */
	  pin_led (GPIO_LEDS_OFF);

	  /* clear UART buffer */
	  UARTCount = 0;
	}
    }
}
