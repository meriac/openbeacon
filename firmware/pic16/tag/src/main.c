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

#include <htc.h>
#include <stdlib.h>
#include "config.h"
#include "timer.h"
#include "nRF_CMD.h"
#include "nRF_HW.h"
#include "openbeacon.h"

__CONFIG (0x03d4);

void
main (void)
{
  unsigned char i,data[NRF_MAC_SIZE];

  /* configure CPU peripherals */
  OPTION_REG = CONFIG_CPU_OPTION;
  TRISA = CONFIG_CPU_TRISA;
  TRISC = CONFIG_CPU_TRISC;
  WPUA = CONFIG_CPU_WPUA;
  ANSEL = CONFIG_CPU_ANSEL;
  CMCON0 = CONFIG_CPU_CMCON0;
  OSCCON = CONFIG_CPU_OSCCON;

  timer_init ();

  for (i = 0; i <= 10; i++)
    {
      CONFIG_PIN_LED = (i & 1) ? 1 : 0;
      sleep_jiffies (JIFFIES_PER_MS (50));
    }

  /* enable RX mode */
  nRFCMD_Init ();
  CONFIG_PIN_CE = 1;
  /* turn on LED */
  CONFIG_PIN_LED = 1;

  IOCA = IOCA | (1 << 0);

  while (1)
    {
	if(!CONFIG_PIN_IRQ)
	{
	  /* disable RX */
	  CONFIG_PIN_CE = 0;

	  /* turn off LED */
	  CONFIG_PIN_LED = 0;

	  /* wait for 2ms to power down RX */
	  sleep_jiffies (JIFFIES_PER_MS (2));

	  nRFCMD_Stop ();

	  while ((nRFCMD_GetFifoStatus () & FIFO_RX_EMPTY) == 0)
	    nRFCMD_RegRead (RD_RX_PLOAD, &data, sizeof (data));

	  sleep_jiffies (JIFFIES_PER_MS (25));

	  /* clear status */
	  nRFCMD_ClearIRQ (MASK_IRQ_FLAGS);

	  /* Start RX again */
	  nRFCMD_Start ();

	  /* turn on LED */
	  CONFIG_PIN_LED = 1;

	  /* enable RX */
	  CONFIG_PIN_CE = 1;
	}
    }
}
