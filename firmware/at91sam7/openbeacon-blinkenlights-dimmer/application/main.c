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
#include <AT91SAM7.h>
#include <USB-CDC.h>
#include <task.h>
#include <beacontypes.h>
#include <board.h>
#include <rnd.h>
#include <dbgu.h>

#include "main.h"
#include "led.h"
#include "proto.h"
#include "usbshell.h"
#include "debug_print.h"
#include "dimmer.h"
#include "env.h"

/**********************************************************************/
#define DIMMER_OFFSET (DIMMER_TICKS/5)
/**********************************************************************/
void
vResetEnv (void)
{
  int i;
  bzero (&env, sizeof (env));

  for (i = 0; i < GAMMA_SIZE; i++)
    env.e.gamma_table[i] = DIMMER_OFFSET +
      (((DIMMER_TICKS - DIMMER_OFFSET) * i) / (GAMMA_SIZE - 1));
}

static inline void
prvSetupHardware (void)
{
  /*  When using the JTAG debugger the hardware is not always initialised to
     the correct default state.  This line just ensures that this does not
     cause all interrupts to be masked at the start. */
  AT91C_BASE_AIC->AIC_EOICR = 0;

  /*  Enable the peripheral clock. */
  AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_PIOA) | (1 << AT91C_ID_PIOB);

  /*  Init LEDs */
  vLedInit ();

  /* If no previous environment exists - create a new, but don't store it */
  env_init ();
  if (!env_load ()) {
    DumpStringToUSB ("unable to load environment, resetting to defaults\n");
    vResetEnv();
  }

  if (env.e.dimmer_delay > 1000)
     env.e.dimmer_delay = 0;
}

/**********************************************************************/
void
vApplicationIdleHook (void)
{
  /* Restart watchdog, has been enabled in Cstartup_SAM7.c */
  AT91F_WDTRestart (AT91C_BASE_WDTC);
}

/**********************************************************************/
void __attribute__((noreturn)) mainloop (void)
{
  prvSetupHardware ();

  xTaskCreate (vUSBCDCTask, (signed portCHAR *) "USB", TASK_USB_STACK,
	       NULL, TASK_USB_PRIORITY, NULL);

  vInitProtocolLayer ( env.e.wmcu_id );
  vUSBShellInit ();
  vRndInit (env.e.mac);
  vInitDimmer ();
  vLedSetGreen (1);
  vTaskStartScheduler ();

  while(1); 
}

