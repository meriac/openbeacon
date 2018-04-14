/***************************************************************
 *
 * OpenBeacon.org - main file for LPC1343 Stroboscope
 *
 * Copyright 2018 Milosch Meriac <milosch@meriac.com>
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

/* 8xLED bar display */
static void
set_led (uint8_t led)
{
	led = ~led;
	LPC_GPIO2->MASKED_ACCESS[0xF0] = led;
	LPC_GPIO3->MASKED_ACCESS[0x0F] = led;
}

static void
strobe (int frequency)
{
	uint32_t period;

	LPC_TMR32B1->TCR = 0;
	if (frequency < 100)
	{
		/* 0.5ms PWM pulse length */
		period = SystemCoreClock / frequency;
		LPC_TMR32B1->MR0 = period - (SystemCoreClock / 2000);
		LPC_TMR32B1->MR3 = period;

		if (LPC_TMR32B1->TC >= period)
			LPC_TMR32B1->TC = period;

		LPC_TMR32B1->TCR = 1;
	}
}

int
main (void)
{
	int mode, increment, frequency;

	/* Get System Clock */
	SystemCoreClockUpdate ();

	/* Initalize PMU */
	pmu_init();

	/* Initialize GPIO (sets up clock) */
	GPIOInit ();
	GPIOSetDir (BUT1_PORT, BUT1_PIN, 0);
	GPIOSetDir (BUT2_PORT, BUT2_PIN, 0);

	/* Set LED port pin to output */
	LPC_GPIO2->DIR |= 0xF0;
	LPC_GPIO3->DIR |= 0x0F;

	/* Set sound port to PIO1_1 and PIO1_2 */
	LPC_GPIO1->DIR |= 0x2;
	LPC_IOCON->JTAG_TDO_PIO1_1 = 3;

	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 10);
	LPC_TMR32B1->TCR = 2;
	LPC_TMR32B1->MCR = 1UL << 10;
	LPC_TMR32B1->PR = 0;
	LPC_TMR32B1->EMR = 0;
	LPC_TMR32B1->PWMC = 1;

	increment = 0;
	frequency = 5;
	strobe(frequency);

	while (1)
	{
		mode = 0;

		/* BUTTON1 press cycles through tones */
		if (!GPIOGetValue (BUT1_PORT, BUT1_PIN))
		{
			set_led (0x01);
			while (!GPIOGetValue (BUT1_PORT, BUT1_PIN));
			set_led (0x00);

			mode |= 0x01;
			increment = increment > 0 ? -1 : 1;
		}

		/* BUTTON2 plays tone */
		if (!GPIOGetValue (BUT2_PORT, BUT2_PIN))
		{
			set_led (0x02);
			while (!GPIOGetValue (BUT2_PORT, BUT2_PIN));
			set_led (0x00);

			mode |= 0x02;
			increment = 0;
		}

		if(increment)
		{
			strobe(frequency);
			frequency += increment;

			if(frequency>100)
				frequency = 100;
			else
				if(frequency<5)
					frequency = 5;
		}

		set_led (mode);
		pmu_wait_ms (490);
		set_led (0x04);
		pmu_wait_ms (10);
	}
}
