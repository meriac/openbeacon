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

enum board_layout_id {
	BOARD_V0_1,
	BOARD_V0_2,
};

struct board_layout {
	enum board_layout_id identifier;
	const char *friendly_name;
	struct {
		AT91PS_PIO pio; unsigned int pin;
	} AD7147_INT, PN532_INT;
};
extern const struct board_layout BOARD_LAYOUTS[];
extern const struct board_layout * BOARD;

/*-------------------------------*/
/* Screen Parameters */
/*-------------------------------*/

#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 800

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
/* AD7147 declaration            */
/*-------------------------------*/
#define AD7147_CS_PIO	AT91C_BASE_PIOB
#define AD7147_CS_PIN	(1L<<10)
#define AD7147_CS		2
#define AD7147_INT_PIO	(BOARD->AD7147_INT.pio)
#define AD7147_INT_PIN	(BOARD->AD7147_INT.pin)

/*-------------------------------*/
/* SD card declaration           */
/*-------------------------------*/
#define SDCARD_CS_PIO	AT91C_BASE_PIOB
#define SDCARD_CS_PIN	(1L<<9)
#define SDCARD_CS		1


/*-------------------------------*/
/* PN532 declaration             */
/*-------------------------------*/
#define PN532_CS_PIO	AT91C_BASE_PIOB
#define PN532_CS_PIN	(1L<<5)
#define PN532_CS		3

#define PN532_INT_PIO  (BOARD->PN532_INT.pio)
#define PN532_INT_PIN  (BOARD->PN532_INT.pin)

/*-------------------------------*/
/* power declaration             */
/*-------------------------------*/

#define POWER_ON_PIN (1L<<14)
#define POWER_ON_PIO AT91C_BASE_PIOB

#define BUTTON_PIN (1L<<15)
#define BUTTON_PIO AT91C_BASE_PIOB

#define POWER_MODE_PIO AT91C_BASE_PIOA
#define POWER_MODE_0_PIN (1L<<21)
#define POWER_MODE_1_PIN (1L<<22)

#define POWER_BATTERY_CHANNEL 7

#define USB_DETECT_PIO AT91C_BASE_PIOA
#define USB_DETECT_PIN (1L<<20)

/*-------------------------------*/
/* eInk (over FPC) declaration   */
/*-------------------------------*/

#define FPC_NHRDY_PIN (1L<<16)
#define FPC_NHRDY_PIO AT91C_BASE_PIOC

#define FPC_NRMODE_PIN (1L<<17)
#define FPC_NRMODE_PIO AT91C_BASE_PIOC

#define FPC_HDC_PIN (1L<<18)
#define FPC_HDC_PIO AT91C_BASE_PIOC

#define FPC_NRESET_PIN (1L<<20)
#define FPC_NRESET_PIO AT91C_BASE_PIOC

#define FPC_SMC_PINS (7L<<21)
#define FPC_SMC_PIO AT91C_BASE_PIOC

#define FPC_HIRQ_PIN (1L<<19)
#define FPC_HIRQ_PIO AT91C_BASE_PIOC


/*-------------------------------*/
/* nRF declaration               */
/*-------------------------------*/

#define DEFAULT_CHANNEL 81

#define CSN_PIN		(1L<<11)
#define MISO_PIN	(1L<<12)
#define MOSI_PIN	(1L<<13)
#define SCK_PIN		(1L<<14)
#define IRQ_PIN		(1L<<19)
#define CE_PIN		(1L<<26)

/*-------------------------------*/
/* acceleromter declaration      */
/*-------------------------------*/
#define ACCELEROMETER_SLEEP_PIN (1L<<12)
#define ACCELEROMETER_SLEEP_PIO AT91C_BASE_PIOB

#define ACCELEROMETER_X_CHANNEL 4
#define ACCELEROMETER_Y_CHANNEL 5
#define ACCELEROMETER_Z_CHANNEL 6


/*-------------------------------*/
/* utils settings                */
/*-------------------------------*/

#define CONFIG_TEA_ENABLEDECODE
#define CONFIG_TEA_ENABLEENCODE

/*-------------------------------*/
/* task priorities               */
/*-------------------------------*/

#define NEAR_IDLE_PRIORITY	( tskIDLE_PRIORITY + 1 )

#define TASK_WATCHDOG_PRIORITY	( NEAR_IDLE_PRIORITY + 1 )
#define TASK_WATCHDOG_STACK		( 128 )

#define TASK_CMD_PRIORITY	( TASK_WATCHDOG_PRIORITY + 1 )
#define TASK_CMD_STACK		( 512 )

#define TASK_USB_PRIORITY	( TASK_WATCHDOG_PRIORITY + 2 )
#define TASK_USB_STACK		( 512 )

#define TASK_NRF_PRIORITY	( TASK_WATCHDOG_PRIORITY + 3 )
#define TASK_NRF_STACK		( 512 )

#define TASK_POWER_PRIORITY	( TASK_WATCHDOG_PRIORITY + 3 )
#define TASK_POWER_STACK	( 128 )

#define TASK_AD7147_PRIORITY	( TASK_WATCHDOG_PRIORITY + 3 )
#define TASK_AD7147_STACK	( 768 )

#define TASK_ACCELEROMETER_PRIORITY	( TASK_WATCHDOG_PRIORITY + 3 )
#define TASK_ACCELEROMETER_STACK	( 128 )

//#define TASK_PN532_PRIORITY	( TASK_WATCHDOG_PRIORITY + 3 )
#define TASK_PN532_PRIORITY	( NEAR_IDLE_PRIORITY )
#define TASK_PN532_STACK	( 256 )

#define TASK_EBOOK_PRIORITY	( NEAR_IDLE_PRIORITY )
#define TASK_EBOOK_STACK	( 512 )

//#define TASK_EINK_PRIORITY	( TASK_WATCHDOG_PRIORITY + 3 )
#define TASK_EINK_PRIORITY	( NEAR_IDLE_PRIORITY )
#define TASK_EINK_STACK	( 256 )

#define OPENPICC_IRQ_PRIO_PIO   (AT91C_AIC_PRIOR_LOWEST+4)

/*-------------------------------*/
/* configuration structure       */
/*-------------------------------*/

#define TENVIRONMENT_MAGIC 0x0CCC2007

typedef struct
{
    unsigned int magic,size,crc16;
    unsigned int mode,speed;
    unsigned int tag_id;
} TEnvironment;

/*----------------------------------*/
/* define debug baud rate if needed */
/*----------------------------------*/

#define AT91C_DBGU_BAUD 115200

#define TXTR_PLEXIGLASS

#endif /*__BOARD_H__*/
