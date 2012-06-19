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
#else  /* no DEBUG enable - remove debug code */
#define debug(...) {}
#endif /*DEBUG*/

#define LED_PORT 1	/* Port for led                      */
#define LED_BIT 9	/* Bit on port for led               */
#define LED_ON 1	/* Level to set port to turn on led  */
#define LED_OFF 0	/* Level to set port to turn off led */

#define ENABLE_USB_FULLFEATURED
#define ENABLE_PN532_RFID

/* USB device settings */
#ifdef  ENABLE_USB_FULLFEATURED
#define USB_VENDOR_ID	0x2366
#define USB_PROD_ID	0x0003
#define USB_DEVICE	0x0100
#endif/*ENABLE_USB_FULLFEATURED*/

/* Clock Definition */
#define SYSTEM_CRYSTAL_CLOCK 12000000
#define SYSTEM_CORE_CLOCK (SYSTEM_CRYSTAL_CLOCK*6)

/* Enable RFID support */
#define ENABLE_PN532_RFID
/* PN532 pin definitions */
#define PN532_RESET_PORT 1
#define PN532_RESET_PIN 11
#define PN532_IRQ_PORT 1
#define PN532_IRQ_PIN 4
#define PN532_CS_PORT 0
#define PN532_CS_PIN 2
#define PN532_SIGOUT_PORT 3
#define PN532_SIGOUT_PIN 2

/* SPI_CS(io_port, io_pin, CPSDVSR frequency, mode) */
#define SPI_CS_PN532 SPI_CS( PN532_CS_PORT, PN532_CS_PIN, 64, SPI_CS_MODE_SKIP_TX|SPI_CS_MODE_BIT_REVERSED )

#endif/*__CONFIG_H__*/
