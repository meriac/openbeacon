/***************************************************************
 *
 * OpenBeacon.org - main entry, CRC, behaviour
 *
 * Copyright 2006 Milosch Meriac <meriac@openbeacon.de>
 *
/***************************************************************

/*
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

#define OPENBEACON_ENABLEENCODE

#include <htc.h>
#include <stdlib.h>
#include "config.h"
#include "timer.h"
#include "openbouncer.h"
#include "protocol.h"
#include "nRF_CMD.h"
#include "nRF_HW.h"
#include "main.h"

__CONFIG (0x03d4);

TMacroBeacon g_MacroBeacon = {
  sizeof (TBouncerEnvelope) + 1,	// Size
  WR_TX_PLOAD			// Transmit Packet Opcode
};

void
main (void)
{
  /* configure CPU peripherals */
  OPTION = CONFIG_CPU_OPTION;
  TRISA = CONFIG_CPU_TRISA;
  TRISC = CONFIG_CPU_TRISC;
  WPUA = CONFIG_CPU_WPUA;
  ANSEL = CONFIG_CPU_ANSEL;
  CMCON0 = CONFIG_CPU_CMCON0;
  OSCCON = CONFIG_CPU_OSCCON;

  timer1_init ();

  sleep_jiffies (0xFFFF);

  nRFCMD_Init ();

  IOCA = IOCA | (1 << 0);

  while (1)
    {

      // light LED during transmission
      if (protocol_setup_hello ())
	{
	  CONFIG_PIN_LED = 1;

	  // send it away
	  nRFCMD_Macro ((u_int8_t *) & g_MacroBeacon);
	  nRFCMD_Execute ();

	  protocol_calc_secret ();
	  protocol_setup_response ();

	  CONFIG_PIN_LED = 0;
	}

      sleep_jiffies (100 * TIMER1_JIFFIES_PER_MS);

    }

  // rest in peace
  while (1)
    sleep_jiffies (0xFFFF);
}
