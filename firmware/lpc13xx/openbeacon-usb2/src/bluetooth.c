/***************************************************************
 *
 * OpenBeacon.org - Bluetooth related functions
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
#include "bluetooth.h"

#define CPU_WAKEUP_BLT_PORT 1
#define CPU_WAKEUP_BLT_PIN 3
#define CPU_BLT_WAKEUP_PORT 2
#define CPU_BLT_WAKEUP_PIN 0
#define CPU_ON_OFF_BLT_PORT 1
#define CPU_ON_OFF_BLT_PIN 0

void
bt_init (void)
{
  /* Init UART for Bluetooth module without RTS/CTS */
  UARTInit (115200, 0);

  /* fake CTS for now */
  GPIOSetDir  ( 1, 5, 1);
  GPIOSetValue( 1, 5, 0);

  /* Set CPU_WAKEUP_BLT port pin to output */
  LPC_IOCON->ARM_SWDIO_PIO1_3=1;
  GPIOSetDir  ( CPU_WAKEUP_BLT_PORT, CPU_WAKEUP_BLT_PIN, 1);
  GPIOSetValue( CPU_WAKEUP_BLT_PORT, CPU_WAKEUP_BLT_PIN, 1);

  /* Set CPU_BLT_WAKEUP port pin to input */
  LPC_IOCON->PIO2_0=0;
  GPIOSetDir  ( CPU_BLT_WAKEUP_PORT, CPU_BLT_WAKEUP_PIN, 0);
  GPIOSetValue( CPU_BLT_WAKEUP_PORT, CPU_BLT_WAKEUP_PIN, 0);

  /* Set CPU_ON-OFF_BLT port pin to output */
  LPC_IOCON->JTAG_TMS_PIO1_0=1;
  GPIOSetDir  ( CPU_ON_OFF_BLT_PORT, CPU_ON_OFF_BLT_PIN, 1);
  GPIOSetValue( CPU_ON_OFF_BLT_PORT, CPU_ON_OFF_BLT_PIN, 1);
}
