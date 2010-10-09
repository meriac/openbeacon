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
int
main (void)
{
  int t;
  volatile int i;
  unsigned char counter;

  /* Initialize GPIO (sets up clock) */
  GPIOInit ();
  /* initialize  pins */
  pin_init ();
  /* setup SPI chipselect pins */
  spi_init_pin (SPI_CS_NRF);
  spi_init_pin (SPI_CS_FLASH);
  spi_init_pin (SPI_CS_ACC3D);

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
  /* UART setup */
//  UARTInit (115200, 0);
  /* Init SPI */
  spi_init ();
  /* Init Bluetooth */
  bt_init ();
  GPIOSetDir  ( 1, 5, 1);
  GPIOSetValue( 1, 5, 0);

  GPIOSetDir  ( 1, 6, 0);
  GPIOSetDir  ( 1, 7, 0);

  /* Hello World */
//  debug_printf ("Hello RFID!\n");

  /* main loop */
  counter = 0;
  while (1)
    {
      /* blink LED0 */
      pin_led (GPIO_LED0);
      for (i = 0; i < 100000; i++);
      pin_led (GPIO_LEDS_OFF);

      /* read firmware revision */
//      debug_printf ("\nreading firmware version (%u)...\n", counter++);

      /* SPI test transmissions */
      spi_txrx (SPI_CS_NRF, NULL, NULL, 16);
      spi_txrx (SPI_CS_FLASH, NULL, NULL, 16);
      spi_txrx (SPI_CS_ACC3D, NULL, NULL, 16);

      /* blink LED1 */
      pin_led (GPIO_LED1);
      for (i = 0; i < 100000; i++);
      pin_led (GPIO_LEDS_OFF);

      /* make device discoverable */
//      debug_printf ("\n\rAT+JDIS=3\n\r");
    }
}
