/***************************************************************
 *
 * OpenBeacon.org - configuration file
 *
 * Copyright 2006 Milosch Meriac <meriac@openbeacon.de>
 *
/***************************************************************

/*
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

#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_DEBUG_LED
#define CONFIG_DELAYUS

/***************************************************************
 *
 *  CPU peripherals settings
 *
 * configuration:
 *      RA0     TX_POWER        ouptput
 *      RA1     SENSOR          input
 *      RA2     nRF_IRQ         input
 *      RA3     Vpp             input
 *      RA4                     input
 *      RA5                     input
 *
 *      RC0     SCK             output
 *      RC1     MOSI            output
 *      RC2     MISO            input
 *      RC3     CSN             output
 *      RC4     nRF_CE          output
 *      RC5     LED             output
 *
 ***************************************************************/

#define CONFIG_PIN_TX_POWER	RA0
#define CONFIG_PIN_SENSOR	RA1
#define CONFIG_PIN_IRQ		RA2

#define CONFIG_PIN_SCK		RC0
#define CONFIG_PIN_MOSI		RC1
#define CONFIG_PIN_MISO		RC2
#define CONFIG_PIN_CSN		RC3
#define CONFIG_PIN_CE		RC4
#define CONFIG_PIN_LED		RC5

#define CONFIG_CPU_XTAL_FREQ	8MHz
#define CONFIG_CPU_TRISA	0x3E
#define CONFIG_CPU_TRISC	0x04
#define CONFIG_CPU_ANSEL	0x00
#define CONFIG_CPU_WPUA		0x02
#define CONFIG_CPU_CMCON0	0x07
#define CONFIG_CPU_OPTION	0x00
#define CONFIG_CPU_OSCCON	0x71

/***************************************************************
 *
 *  RF chip settings
 *
 ***************************************************************/

#define CONFIG_DEFAULT_CHANNEL 81

#undef  CONFIG_HIRES_LOCATION

#ifdef  CONFIG_HIRES_LOCATION
#define CONFIG_MAX_POWER_LEVELS 8
#else /*CONFIG_HIRES_LOCATION */
#define CONFIG_MAX_POWER_LEVELS 4
#endif /*CONFIG_HIRES_LOCATION */

#endif
