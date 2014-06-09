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

#define BUT1_PORT 2		/* Port for Button 1       */
#define BUT1_PIN 9		/* Bit on port forButton 1 */
#define BUT2_PORT 1		/* Port for Button 2       */
#define BUT2_PIN 4		/* Bit on port forButton 2 */

#define ENABLE_USB_FULLFEATURED

/* enable USB disk support */
#ifdef  ENABLE_USB_FULLFEATURED
#define USB_VENDOR_ID 0x2366
#define USB_PROD_ID 0x0003
#define USB_DEVICE 1
#endif /*ENABLE_USB_FULLFEATURED */

/* Clock Definition */
#define SYSTEM_CRYSTAL_CLOCK 12000000
#define SYSTEM_CORE_CLOCK (SYSTEM_CRYSTAL_CLOCK*6)

#endif/*__CONFIG_H__*/
