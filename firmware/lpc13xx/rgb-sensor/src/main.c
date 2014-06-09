/***************************************************************
 *
 * OpenBeacon.org - main file for OpenBeacon USB II Bluetooth
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
#include <openbeacon.h>
#include "usbserial.h"

#define LED_OFF 0x00
#define LED_R   0x01
#define LED_G   0x02
#define LED_B   0x04

static inline void set_rgb(uint8_t rgb)
{
	LPC_GPIO1->MASKED_ACCESS[0xE] = rgb << 1;
}

static void init_hardware(void)
{
	/* init wait timer */
	pmu_init();

	/* Initialize GPIO (sets up clock) */
	GPIOInit ();
	GPIOSetDir (BUT1_PORT, BUT1_PIN, 0);
	GPIOSetDir (BUT2_PORT, BUT2_PIN, 0);

	/* initialize LED RGB port */
	LPC_GPIO1->DIR = 0x0E;
	LPC_IOCON->JTAG_TDO_PIO1_1   = 0x81;
	LPC_IOCON->JTAG_nTRST_PIO1_2 = 0x81;
	LPC_IOCON->ARM_SWDIO_PIO1_3  = 0x81;

	/* CDC USB Initialization */
	init_usbserial ();
}

int
main (void)
{
	init_hardware();

	while(1)
	{
		set_rgb(LED_R);
		pmu_wait_ms(500);
		set_rgb(LED_G);
		pmu_wait_ms(500);
		set_rgb(LED_B);
		pmu_wait_ms(500);
		set_rgb(LED_OFF);
		pmu_wait_ms(1000);

		debug_printf("Hello World\n\r");
	}

	return 0;
}
