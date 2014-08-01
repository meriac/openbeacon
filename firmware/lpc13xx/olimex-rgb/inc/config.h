/***************************************************************
 *
 * OpenBeacon.org - config file
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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef  DEBUG
#define debug(args...) debug_printf(args)
#else /* no DEBUG enable - remove debug code */
#define debug(...) {}
#endif /*DEBUG*/

#define LED_PORT 3																/* Port for led                      */
#define LED_PIN0 0																/* Bit on port for led               */
#define LED_PIN1 1																/* Bit on port for led               */
#define LED_ON 0																/* Level to set port to turn on led  */
#define LED_OFF 1																/* Level to set port to turn off led */

#define EMETER_PORT 1
#define EMETER_PIN 5

#define ENABLE_USB_FULLFEATURED
#define UART_DISABLE

/* USB device settings */
#ifdef  ENABLE_USB_FULLFEATURED
#define USB_VENDOR_ID	0x2366
#define USB_PROD_ID	0x0003
#define USB_DEVICE	0x0100
#endif /*ENABLE_USB_FULLFEATURED */

/* Clock Definition */
#define SYSTEM_CRYSTAL_CLOCK 12000000
#define SYSTEM_CORE_CLOCK (SYSTEM_CRYSTAL_CLOCK*6)

#endif/*__CONFIG_H__*/
