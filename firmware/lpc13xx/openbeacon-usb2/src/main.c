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
#include "iap.h"
#include "pmu.h"
#include "bluetooth.h"
#include "3d_acceleration.h"
#include "storage.h"
#include "nRF_API.h"
#include "nRF_CMD.h"

// set nRF24L01 broadcast mac
const unsigned char broadcast_mac[NRF_MAX_MAC_SIZE] = { 1, 2, 3, 2, 1 };

#if (USB_HID_IN_REPORT_SIZE>0)||(USB_HID_OUT_REPORT_SIZE>0)
static uint8_t hid_buffer[USB_HID_IN_REPORT_SIZE];

void
GetInReport (uint8_t *src, uint32_t length)
{
  (void) src;
  (void) length;

  if (length > USB_HID_IN_REPORT_SIZE)
    length = USB_HID_IN_REPORT_SIZE;

  memcpy (src, hid_buffer, length);
}

void
SetOutReport (uint8_t *dst, uint32_t length)
{
  (void) dst;
  (void) length;
}
#endif

static void
show_version (void)
{
  TDeviceUID uid;

  memset(&uid,0,sizeof(uid));
  iap_read_uid(&uid);

  debug_printf (" * Device UID: %08X:%08X:%08X:%08X\n",uid[0],uid[1],uid[2],uid[3]);
}

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
		    " * P            - invoke ISP programming mode\n"
		    " * S            - SPI status\n"
		    " * R            - OpenBeacon nRF24L01 register dump\n"
		    " *****************************************************\n"
		    "\n");
      break;
    case 'P':
      debug_printf ("\nRebooting...");
      iap_invoke_isp ();
      break;
    case 'R':
      nRFCMD_RegisterDump ();
      break;
    case 'S':
      debug_printf ("\n"
		    " *****************************************************\n"
		    " * OpenBeacon Status Information                     *\n"
		    " *****************************************************\n");
      show_version ();
      spi_status ();
      acc_status ();
//      pmu_status ();
#if (DISK_SIZE>0)
      storage_status ();
#endif
      nRFCMD_Status ();
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

  /* initialize  pins */
  pin_init ();
  /* Init SPI */
  spi_init ();
  /* Init Storage */
#if (DISK_SIZE>0)
  storage_init ();
#endif
  /* Init USB HID interface */
#if (USB_HID_IN_REPORT_SIZE>0)||(USB_HID_OUT_REPORT_SIZE>0)
  hid_init ();
#endif
  /* power management init */
  pmu_init ();

  /* blink as a sign of boot to detect crashes */
  for (t = 0; t < 10; t++)
    {
      pin_led (GPIO_LED0);
	  for (i = 0; i < 100000; i++);
      pin_led (GPIO_LEDS_OFF);
	  for (i = 0; i < 100000; i++);
    }

  /* Init Bluetooth */
  bt_init ();
  /* Init 3D acceleration sensor */
  acc_init ();

  /* Init OpenBeacon nRF24L01 interface */
//  nRFAPI_Init (81, broadcast_mac, sizeof (broadcast_mac), 0);

  /* main loop */
  t = 0;
  firstrun = 1;


  while (1)
  {
      pin_led (GPIO_LEDS_OFF);
    pmu_off();
      pin_led (GPIO_LED0);
    pmu_off();
  }
#if 0
  }
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
#endif
}
