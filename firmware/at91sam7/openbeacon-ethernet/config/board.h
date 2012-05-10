 /***************************************************************
 *
 * OpenBeacon.org - board specific configuration
 *
 * Copyright 2007 Milosch Meriac <meriac@openbeacon.de>
 *
 * change this file to reflect hardware design changes
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
#ifndef __BOARD_H__
#define __BOARD_H__

#include <lib_AT91SAM7X.h>
#include <beacontypes.h>
#include <lwip/ip.h>

#define VERSION		"1.2"
#define VERSION_INT	0x00010002

/*---------------------------------*/
/* SD-card-sector-size             */
/*---------------------------------*/
#define SECTOR_SIZE 512

/*---------------------------------*/
/* SAM7Board Memories Definition   */
/*                                 */
/* if changing EVIRONMENT_SIZE     */
/* make sure to adapt atmel-rom.ld */
/*---------------------------------*/

#define ENVIRONMENT_SIZE	1024
#define	FLASH_PAGE_NB		AT91C_IFLASH_NB_OF_PAGES-(ENVIRONMENT_SIZE/AT91C_IFLASH_PAGE_SIZE)

/*-------------------------------*/
/* Master Clock                  */
/*-------------------------------*/

#define EXT_OC		18432000													// Exetrnal ocilator MAINCK
#define MCK		47923200														// MCK (PLLRC div by 2)
#define MCKKHz		(MCK/1000)													//

/*-------------------------------*/
/* LED declaration               */
/*-------------------------------*/

#define LED_PIO         	AT91C_BASE_PIOB
#define LED_GREEN       	(1L<<27)
#define LED_RED         	(1L<<28)
#define LED_MASK        	(LED_GREEN|LED_RED)

/*-------------------------------*/
/* nRF declaration               */
/*-------------------------------*/

#define DEFAULT_CHANNEL 81

#define OB0_CSN_ID      	0
#define OB0_IRQ_PIN     	(1L<<14)
#define OB0_CE_PIN      	(1L<<19)
#define OB0_CSN_PIN     	(1L<<21)
#define OB0_LED_BEACON_GREEN	(1L<<6)
#define OB0_LED_BEACON_RED	(1L<<7)

#define OB1_CSN_ID      	3
#define OB1_IRQ_PIN     	(1L<<30)
#define OB1_CE_PIN      	(1L<<20)
#define OB1_CSN_PIN     	(1L<<29)
#define OB1_LED_BEACON_GREEN	(1L<<9)
#define OB1_LED_BEACON_RED	(1L<<8)

#define SCK_PIN         	(1L<<22)
#define MOSI_PIN        	(1L<<23)
#define MISO_PIN        	(1L<<24)

#define LED_BEACON_PIO		AT91C_BASE_PIOA
#define LED_BEACON_MASK		(OB0_LED_BEACON_GREEN|OB0_LED_BEACON_RED|OB1_LED_BEACON_GREEN|OB1_LED_BEACON_RED)

/*-------------------------------*/
/* External Button               */
/*-------------------------------*/
#define EXT_BUTTON_PIO		AT91C_BASE_PIOB
#define EXT_BUTTON_PIN		(1L<<29)

/*-------------------------------*/
/* utils settings                */
/*-------------------------------*/

#define CONFIG_TEA_ENABLEDECODE
#define CONFIG_TEA_ENABLEENCODE

/*-------------------------------*/
/* task priorities               */
/*-------------------------------*/

#define TASK_CMD_PRIORITY	( tskIDLE_PRIORITY + 1 )
#define TASK_CMD_STACK		( 256 )

#define TASK_USB_PRIORITY	( tskIDLE_PRIORITY + 2 )
#define TASK_USB_STACK		( 256 )

#define TASK_FILE_PRIORITY	( tskIDLE_PRIORITY + 3 )
#define TASK_FILE_STACK		( 256 )

#define TASK_NET_PRIORITY	( tskIDLE_PRIORITY + 4 )
#define TASK_NET_STACK		( 2048 )

#define TASK_NRF_PRIORITY	( tskIDLE_PRIORITY + 5 )
#define TASK_NRF_STACK		( 256 )

/*-------------------------------*/
/* configuration structure       */
/*-------------------------------*/

#define TENVIRONMENT_MAGIC 0x7F12971D

/*-------------------------------*/
/*  Bitmanufaktur GmbH MAC IAB   */
/*    registered at ieee.org     */
/*-------------------------------*/
#define MAC_OID 0x0050C2UL
#define MAC_IAB 0xAB1000UL
#define MAC_IAB_MASK 0xFFFUL
/*-------------------------------*/
#define IP_AUTOCONFIG_STATIC_IP 0
#define IP_AUTOCONFIG_READER_ID 1
#define IP_AUTOCONFIG_DHCP 2
/*-------------------------------*/
#define DEFAULT_SERVER_PORT 2342
/*-------------------------------*/

typedef struct
{
	unsigned int magic, size, crc16;
	unsigned int reader_id, ip_autoconfig, ip_server_port;
	unsigned int filter_duplicates;
	struct ip_addr ip_host, ip_netmask, ip_server, ip_gateway;
} TEnvironment __attribute__ ((aligned (4)));

/*----------------------------------*/
/* define debug baud rate if needed */
/*----------------------------------*/

#define AT91C_DBGU_BAUD 115200

/*-------------------------------*/
/* SD card declaration           */
/*-------------------------------*/
#define SDCARD_CS_PIO	AT91C_BASE_PIOA
#define SDCARD_CS_PIN	(1L<<12)
#define SDCARD_CS	0

/*-------------------------------*/
/* SPI declaration               */
/*-------------------------------*/
#define SD_SPI_PIO	AT91C_BASE_PIOA
#define SD_SPI_MISO	(1L<<16)
#define SD_SPI_MOSI	(1L<<17)
#define SD_SPI_SCK	(1L<<18)

#endif /*__BOARD_H__*/
