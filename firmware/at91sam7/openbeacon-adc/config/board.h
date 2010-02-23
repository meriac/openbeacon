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

#define LED_PIO		AT91C_BASE_PIOA
#define LED_RED		(1L<<17)
#define LED_GREEN	(1L<<18)
#define LED_MASK	(LED_GREEN|LED_RED)

/*-------------------------------*/
/* PWM audio declaration         */
/*-------------------------------*/

#define ADC_CLOCK_FREQUENCY	200000
#define ADC_CLOCK_PIO		AT91C_BASE_PIOA
#define ADC_CLOCK_PIN		(1L<<0)

/*-------------------------------*/
/* task priorities               */
/*-------------------------------*/

#define TASK_NRF_PRIORITY	( tskIDLE_PRIORITY + 2 )
#define TASK_NRF_STACK		( 512 )

/*-------------------------------*/
/* configure serial port         */
/*-------------------------------*/

#define AT91C_DBGU_BAUD 230400

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

#endif /*__BOARD_H__*/
