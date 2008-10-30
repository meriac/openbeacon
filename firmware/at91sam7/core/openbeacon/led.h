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

#ifndef __LED_H__
#define __LED_H__

#if defined(LED_RED)
extern void led_set_red(bool_t on);
#endif

#if defined(LED_GREEN)
extern void led_set_green(bool_t on);
#endif

extern void led_halt_blinking(int reason);
extern void led_init(void);

/* Legacy names */
#define vLedSetRed       led_set_red
#define vLedSetGreen     led_set_green
#define vLedHaltBlinking led_halt_blinking
#define vLedInit         led_init

#endif/*__LED_H__*/
