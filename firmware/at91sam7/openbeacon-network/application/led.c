/***************************************************************
 *
 * OpenBeacon.org - LED support
 *
 * Copyright 2007 Milosch Meriac <meriac@openbeacon.de>
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
#include <FreeRTOS.h>
#include <board.h>
#include <string.h>
#include <task.h>
#include "led.h"
/**********************************************************************/

void
vLedSetRed (bool_t on)
{
  if (on)
    AT91F_PIO_ClearOutput (AT91C_BASE_PIOB, LED_RED);
  else
    AT91F_PIO_SetOutput (AT91C_BASE_PIOB, LED_RED);
}

/**********************************************************************/

extern void
vLedSetGreen (bool_t on)
{
  if (on)
    AT91F_PIO_ClearOutput (AT91C_BASE_PIOB, LED_GREEN);
  else
    AT91F_PIO_SetOutput (AT91C_BASE_PIOB, LED_GREEN);
}

/**********************************************************************/

void
vLedHaltBlinking (int reason)
{

  volatile u_int32_t i = 0;
  s_int32_t t;
  while (1)
    {
      for (t = 0; t < reason; t++)
	{
	  AT91F_PIO_ClearOutput (AT91C_BASE_PIOB, LED_MASK);
	  for (i = 0; i < MCK / 200; i++)
	    AT91F_WDTRestart (AT91C_BASE_WDTC);

	  AT91F_PIO_SetOutput (AT91C_BASE_PIOB, LED_MASK);
	  for (i = 0; i < MCK / 100; i++)
	    AT91F_WDTRestart (AT91C_BASE_WDTC);

	}
      for (i = 0; i < MCK / 25; i++)
	AT91F_WDTRestart (AT91C_BASE_WDTC);
    }
}

/**********************************************************************/

void
vLedInit (void)
{
  // turn off LED's 
  AT91F_PIO_CfgOutput (AT91C_BASE_PIOB, LED_MASK);
  AT91F_PIO_SetOutput (AT91C_BASE_PIOB, LED_MASK);
}
