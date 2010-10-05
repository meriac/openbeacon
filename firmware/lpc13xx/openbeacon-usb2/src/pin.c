/***************************************************************
 *
 * OpenBeacon.org - GPIO declaration
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

#define LED0_PORT 1 /* Port for led                      */
#define LED0_BIT 2  /* Bit on port for led               */
#define LED1_PORT 1 /* Port for led                      */
#define LED1_BIT 1  /* Bit on port for led               */

void
pin_led(uint8_t led)
{
    GPIOSetValue( LED0_PORT, LED0_BIT,  led & GPIO_LED0    );
    GPIOSetValue( LED1_PORT, LED1_BIT, (led & GPIO_LED1)>1 );
}

void
pin_init (void)
{
  /* Initialize GPIO (sets up clock) */
  GPIOInit ();

  /* Set LED0 port pin to output */
  LPC_IOCON->JTAG_nTRST_PIO1_2=1;
  GPIOSetValue( LED0_PORT, LED0_BIT, 0);
  GPIOSetDir (LED0_PORT, LED0_BIT, 1);

  /* Set LED1 port pin to output */
  LPC_IOCON->JTAG_TDO_PIO1_1=1;
  GPIOSetValue( LED1_PORT, LED1_BIT, 0);
  GPIOSetDir (LED1_PORT, LED1_BIT, 1);
}
