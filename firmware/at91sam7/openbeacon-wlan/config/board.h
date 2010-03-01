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

#include <lib_AT91SAM7.h>

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

#define EXT_OC		18432000	// Exetrnal ocilator MAINCK
#define MCK		47923200	// MCK (PLLRC div by 2)
#define MCKKHz		(MCK/1000)	//

/*-------------------------------*/
/* LED declaration               */
/*-------------------------------*/

#define LED_PIO		AT91C_BASE_PIOB
#define LED_RED		(1L<<0)
#define LED_GREEN	(1L<<1)
#define LED_MASK	(LED_GREEN|LED_RED)

/*-------------------------------*/
/* SPI declaration               */
/*-------------------------------*/
#define SPI_PIO		AT91C_BASE_PIOA
#define SPI_MISO 	(1L<<12)
#define SPI_MOSI	(1L<<13)
#define SPI_SCK		(1L<<14)

/*-------------------------------*/
/* SD card declaration           */
/*-------------------------------*/
#define SDCARD_CS_PIO	AT91C_BASE_PIOA
#define SDCARD_CS_PIN	(1L<<26)
#define SDCARD_CS	1

/*-------------------------------*/
/* power declaration             */
/*-------------------------------*/

#define USB_DETECT_PIO AT91C_BASE_PIOA
#define USB_DETECT_PIN	(1L<<20)

/*-------------------------------*/
/* serial port WLAN declaration  */
/*-------------------------------*/

#define WLAN_BAUDRATE		921600

#define WLAN_PIO AT91C_BASE_PIOA
#define WLAN_ADHOC	(1L<<4)
#define WLAN_RXD	(1L<<5)
#define WLAN_TXD	(1L<<6)
#define WLAN_RTS	(1L<<7)
#define WLAN_CTS	(1L<<8)
#define WLAN_RESET	(1L<<21)
#define WLAN_WAKE	(1L<<22)

/*-------------------------------*/
/* nRF declaration               */
/*-------------------------------*/

#define DEFAULT_CHANNEL 81
#define NRF_SECONDARY

#ifdef NRF_PRIMARY
#define CSN_PIN_PIO	AT91C_BASE_PIOB
#define CSN_PIN		(1L<<3)
#define CSN_ID		(3)
#define CE_PIN		(1L<<15)
#define IRQ_PIN		(1L<<16)
#endif/*NRF_PRIMARY*/

#ifdef  NRF_SECONDARY
#define CSN_PIN_PIO	AT91C_BASE_PIOB
#define CSN_PIN		(1L<<9)
#define CSN_ID		(1)
#define CE_PIN		(1L<<19)
#define IRQ_PIN		(1L<<17)
#endif/*NRF_SECONDARY*/

#if defined (NRF_PRIMARY) || defined (NRF_SECONDARY)
#define MISO_PIN	(1L<<12)
#define MOSI_PIN	(1L<<13)
#define SCK_PIN		(1L<<14)
#else
#error please define either NRF_PRIMARY or NRF_SECONDARY to select interface
#endif/*NRF_PRIMARY || NRF_SECONDARY*/

/*-------------------------------*/
/* utils settings                */
/*-------------------------------*/

#define CONFIG_TEA_ENABLEDECODE
#define CONFIG_TEA_ENABLEENCODE

/*-------------------------------*/
/* task priorities               */
/*-------------------------------*/

#define NEAR_IDLE_PRIORITY	( tskIDLE_PRIORITY + 1 )

#define TASK_USB_PRIORITY	( NEAR_IDLE_PRIORITY + 2 )
#define TASK_USB_STACK		( 512 )

#define TASK_UART_PRIORITY	( NEAR_IDLE_PRIORITY + 2 )
#define TASK_UART_STACK		( 512 )

#define TASK_NRF_PRIORITY	( NEAR_IDLE_PRIORITY + 3 )
#define TASK_NRF_STACK		( 512 )

/*-------------------------------*/
/* configuration structure       */
/*-------------------------------*/

#define TENVIRONMENT_MAGIC 0x0CCC2007

typedef struct
{
    unsigned int magic,size,crc16;
    unsigned int mode,speed;
    unsigned int reader_id;
} TEnvironment;

/*----------------------------------*/
/* define debug baud rate if needed */
/*----------------------------------*/

#define AT91C_DBGU_BAUD 115200

#define LOWLEVEL_BOARD_INIT sdram_init()

#include "sdram.h"

#endif /*__BOARD_H__*/
