/***************************************************************
 *
 * OpenBeacon.org - main entry for 2.4GHz RFID USB reader
 *
 * Copyright 2007 Milosch Meriac <meriac@openbeacon.de>
 *
 * basically starts the USB task, initializes all IO ports
 * and introduces idle application hook to handle the HF traffic
 * from the nRF24L01 chip
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
/* Library includes. */
#include <string.h>
#include <stdio.h>

#include <FreeRTOS.h>
#include <board.h>
#include <USB-CDC.h>
#include <task.h>
#include <dbgu.h>
#include <beacontypes.h>
#include <proto.h>
#include <rnd.h>
#include <network.h>
#include <debug_printf.h>

#include "led.h"
#include "proto.h"
#include "usbshell.h"
#include "env.h"

/**********************************************************************/
static inline void
prvSetupHardware (void)
{
  /*  When using the JTAG debugger the hardware is not always initialised to
     the correct default state.  This line just ensures that this does not
     cause all interrupts to be masked at the start. */
  AT91C_BASE_AIC->AIC_EOICR = 0;

  /*  Enable the peripheral clock. */
  AT91C_BASE_PMC->PMC_PCER =
    (1 << AT91C_ID_PIOA) |
    (1 << AT91C_ID_PIOB) |
    (1 << AT91C_ID_EMAC) |
    (1 << AT91C_ID_SPI1);
}

/**********************************************************************/
void
vApplicationIdleHook (void)
{
#ifndef DISABLE_WATCHDOG
  /* Restart watchdog, has been enabled in Cstartup_SAM7.c */
  AT91F_WDTRestart (AT91C_BASE_WDTC);
#endif /*DISABLE_WATCHDOG */
}
/**********************************************************************/
void
vDebugSendHook (char data)
{
    vUSBSendByte (data);
}

/**********************************************************************/
void __attribute__((noreturn)) mainloop (void)
{
  prvSetupHardware ();
  vLedInit ();

  /* If no previous environment exists - create a new, but don't store it */
  env_init ();
  if(!env_load ()) {
    debug_printf ("unable to load environment, resetting to defaults\n");
    bzero (&env, sizeof (env));
  }

  if (env.e.n_lamps > MAX_LAMPS)
    env.e.n_lamps = 0;

  vRndInit ((((u_int32_t) env.e.mac_h) << 8) | env.e.mac_l);
  vNetworkInit ();

  xTaskCreate (vUSBCDCTask, (signed portCHAR *) "USB", TASK_USB_STACK,
	       NULL, TASK_USB_PRIORITY, NULL);

  PtInitProtocol ();
  vUSBShellInit ();
  vTaskStartScheduler ();

  while(1);
}
