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

/* enable USB disk support */
#define USB_DISK_SUPPORT
#define USB_VENDOR_ID 0x2366
#define USB_PROD_ID 0x0003
#define USB_DEVICE 1

/* SPI_CS(io_port, io_pin, frequency_in_MHz, mode) */
#define SPI_CS_FLASH SPI_CS( 1, 8, 80.0, SPI_CS_MODE_NORMAL )
#define SPI_CS_NRF   SPI_CS( 1,10,  8.0, SPI_CS_MODE_NORMAL )
#define SPI_CS_ACC3D SPI_CS( 0, 4, 10.0, SPI_CS_MODE_NORMAL )

#define NRF_MAX_MAC_SIZE 5

#endif/*__CONFIG_H__*/
