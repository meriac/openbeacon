/***************************************************************
 *
 * OpenBeacon.org - main entry, CRC, behaviour
 *
 * Copyright 2006 Milosch Meriac <meriac@openbeacon.de>
 *
 ***************************************************************/

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

#define OPENBEACON_ENABLEENCODE

#include <htc.h>
#include <stdlib.h>
#include "timer.h"
#include "main.h"
#include "config.h"

__CONFIG (0x03d4);

void
main (void)
{
	char data, *c, morsemessage[] = "... ___ ...";

	/* configure CPU peripherals */
	OPTION_REG = CONFIG_CPU_OPTION;
	TRISA = CONFIG_CPU_TRISA;
	TRISC = CONFIG_CPU_TRISC;
	WPUA = CONFIG_CPU_WPUA;
	ANSEL = CONFIG_CPU_ANSEL;
	CMCON0 = CONFIG_CPU_CMCON0;
	OSCCON = CONFIG_CPU_OSCCON;

	timer1_init ();

	IOCA = IOCA | (1 << 0);

#define GAP 100
	while (1)
	{
		c = morsemessage;
		while (data = (*c++))
		{
			switch (data)
			{
				case '.':
					CONFIG_PIN_LED = 1;
					sleep_jiffies (1 * GAP * TIMER1_JIFFIES_PER_MS);
					CONFIG_PIN_LED = 0;
					break;

				case ' ':
					sleep_jiffies (3 * GAP * TIMER1_JIFFIES_PER_MS);
					break;

				case '_':
					CONFIG_PIN_LED = 1;
					sleep_jiffies (3 * GAP * TIMER1_JIFFIES_PER_MS);
					CONFIG_PIN_LED = 0;
					break;
			}
			sleep_jiffies (1 * GAP * TIMER1_JIFFIES_PER_MS);
		}
		sleep_jiffies (10 * GAP * TIMER1_JIFFIES_PER_MS);
	}
}
