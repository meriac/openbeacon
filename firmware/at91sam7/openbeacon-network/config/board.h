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
#include "proto.h"

#define VERSION		"1.0"
#define VERSION_INT	0x00010000

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

#define LED_PIO         AT91C_BASE_PIOB
#define LED_GREEN       (1L<<27)
#define LED_RED         (1L<<28)
#define LED_MASK        (LED_GREEN|LED_RED)

/*-------------------------------*/
/* nRF declaration               */
/*-------------------------------*/

#define DEFAULT_CHANNEL 81

#define IRQ_PIN         (1L<<14)
#define CSN_PIN         (1L<<21)
#define SCK_PIN         (1L<<22)
#define MOSI_PIN        (1L<<23)
#define MISO_PIN        (1L<<24)
#define CE_PIN          (1L<<29)

/*-------------------------------*/
/* utils settings                */
/*-------------------------------*/

#define CONFIG_TEA_ENABLEDECODE
#define CONFIG_TEA_ENABLEENCODE

/*-------------------------------*/
/* task priorities               */
/*-------------------------------*/

#define TASK_USBSHELL_PRIORITY	( tskIDLE_PRIORITY)
#define TASK_USBSHELL_STACK	( 512 )

#define TASK_USB_PRIORITY	( tskIDLE_PRIORITY + 2 )
#define TASK_USB_STACK		( 512 )

#define TASK_NRF_PRIORITY	( tskIDLE_PRIORITY + 3 )
#define TASK_NRF_STACK		( 128 )

#define TASK_NET_PRIORITY	( tskIDLE_PRIORITY + 4 )
#define TASK_NET_STACK		( 1024 )

/*-------------------------------*/
/* configuration structure       */
/*-------------------------------*/

#define TENVIRONMENT_MAGIC 0x2DECADE
#define MAX_LAMPS (RF_PAYLOAD_SIZE * 2)

extern unsigned char last_lamp_val[MAX_LAMPS];

typedef struct
{
  unsigned char flags, screen, x, y;
  unsigned short mac;
} PACKED LampMap;

typedef struct
{
  unsigned int magic, size, crc16;
  signed int mcu_id;
  unsigned int n_lamps;
  LampMap lamp_map[MAX_LAMPS];
  unsigned char mac_h, mac_l;
} PACKED __attribute__((aligned (4))) TEnvironment;

/*----------------------------------*/
/* define debug baud rate if needed */
/*----------------------------------*/

#define AT91C_DBGU_BAUD 115200

#endif /*__BOARD_H__*/
