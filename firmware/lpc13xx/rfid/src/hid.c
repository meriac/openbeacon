/***************************************************************
 *
 * OpenBeacon.org - HID ROM function code for LPC13xx
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

#define     EN_TIMER32_1    (1<<10)
#define     EN_IOCON        (1<<16)
#define     EN_USBREG       (1<<14)

const static ROM **rom = (ROM **) 0x1fff1ff8;

void
USB_IRQHandler (void)
{
  (*rom)->pUSBD->isr ();
}

void
hid_init (void)
{
  volatile int i;
  static USB_DEV_INFO DeviceInfo;
  static HID_DEVICE_INFO HidDevInfo;

  /* Setup ROM initialization structure */
  HidDevInfo.idVendor = USB_VENDOR_ID;
  HidDevInfo.idProduct = USB_PROD_ID;
  HidDevInfo.bcdDevice = USB_DEVICE;
  HidDevInfo.StrDescPtr = (uint32_t) & USB_StringDescriptor[0];
  HidDevInfo.InReportCount = 1;
  HidDevInfo.OutReportCount = 1;
  HidDevInfo.SampleInterval = 0x20;
  HidDevInfo.InReport = GetInReport;
  HidDevInfo.OutReport = SetOutReport;

  /* Point DeviceInfo to HidDevInfo */
  DeviceInfo.DevType = USB_DEVICE_CLASS_HUMAN_INTERFACE;
  DeviceInfo.DevDetailPtr = (uint32_t) & HidDevInfo;

  /* Enable Timer32_1, IOCON, and USB blocks (for USB ROM driver) */
  LPC_SYSCON->SYSAHBCLKCTRL |= (EN_TIMER32_1 | EN_IOCON | EN_USBREG);

  /* Use pll and pin init function in rom */
  (*rom)->pUSBD->init_clk_pins ();

  /* insert delay between init_clk_pins() and usb init */
  for (i = 0; i < 75; i++);

  /* USB Initialization ... */
  (*rom)->pUSBD->init (&DeviceInfo);

  /* ... and USB Connect */
  (*rom)->pUSBD->connect (TRUE);
}
