/***************************************************************
 *
 * OpenBeacon.org - LED support
 *
 * Copyright 2007 Milosch Meriac <meriac@openbeacon.de>
 *           2008 Henryk Plötz <henryk@ploetzli.ch>
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
#include <beacontypes.h>
#include <board.h>
#include <task.h>
#include <led.h>
/**********************************************************************/

static inline void
led_set(AT91PS_PIO pio, const u_int32_t led, const bool_t on)
{
	if (on)
		AT91F_PIO_ClearOutput(pio, led);
	else
		AT91F_PIO_SetOutput(pio, led);
}

#if defined(LED_RED)
void led_set_red(bool_t on)
{
	led_set(LED_PIO, LED_RED, on);
}
#endif

/**********************************************************************/

#if defined(LED_GREEN)
void led_set_green(bool_t on)
{
	led_set(LED_PIO, LED_GREEN, on);
}
#endif

/**********************************************************************/

void led_halt_blinking(int reason)
{
	volatile u_int32_t i = 0;
	s_int32_t t;
	while (1) {
		for (t = 0; t < reason; t++) {
			led_set(LED_PIO, LED_MASK, 1);
			for (i = 0; i < MCK / 200; i++)
				AT91F_WDTRestart(AT91C_BASE_WDTC);

			led_set(LED_PIO, LED_MASK, 0);
			for (i = 0; i < MCK / 100; i++)
				AT91F_WDTRestart(AT91C_BASE_WDTC);
		}
		for (i = 0; i < MCK / 25; i++)
			AT91F_WDTRestart(AT91C_BASE_WDTC);
	}
}

/**********************************************************************/

void led_init(void)
{
	// turn off LEDs
	AT91F_PIO_CfgOutput(LED_PIO, LED_MASK);
	AT91F_PIO_SetOutput(LED_PIO, LED_MASK);
}
