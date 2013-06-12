/***************************************************************
 *
 * OpenBeacon.org - eMeter demo
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

volatile uint32_t timer_old, pulse_length, pulse_count;

void
CDC_GetCommand (unsigned char *command)
{
	(void) command;
}

void
TIMER32_0_IRQHandler (void)
{
	uint32_t irq_reason, cr;

	/* get IRQ reason */
	irq_reason = LPC_TMR32B0->IR;

	/* capture IRQ event */
	if((irq_reason & 0x10)>0)
	{
		cr = LPC_TMR32B0->CR0;
		pulse_length = cr-timer_old;
		timer_old = cr;
		pulse_count++;

		/* update LED */
		GPIOSetValue (LED_PORT, LED_PIN1, pulse_count&1);
	}

	/* reset IRQ reason */
	LPC_TMR32B0->IR = irq_reason;
}

int
main (void)
{
	/* Initialize GPIO (sets up clock) */
	GPIOInit ();

	/* Set eMeter port pin to input/pullup/hysteresis/CAP0 */
	LPC_IOCON->PIO1_5 = 2| 2<<3 | 1<<5;
	GPIOSetDir (EMETER_PORT, EMETER_PIN, 0);
	GPIOSetValue (EMETER_PORT, EMETER_PIN, 0);

	/* Set LED port pin to output */
	GPIOSetDir (LED_PORT, LED_PIN0, 1);
	GPIOSetDir (LED_PORT, LED_PIN1, 1);
	GPIOSetValue (LED_PORT, LED_PIN0, LED_OFF);
	GPIOSetValue (LED_PORT, LED_PIN1, LED_OFF);

	/* Init Power Management Routines */
	pmu_init ();

	/* CDC USB Initialization */
	init_usbserial ();


	/* configure TMR32B0 for capturing eMeter pulse duration */
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 9);
	LPC_TMR32B0->TCR = 2;
	LPC_TMR32B0->CCR = 6;

	/* enable TMR32B0 timer IRQ */
	pulse_count = pulse_length = 0;
	timer_old = LPC_TMR32B0->TC;
	NVIC_EnableIRQ(TIMER_32_0_IRQn);

	/* release counter */
	LPC_TMR32B0->TCR = 1;

	while(TRUE)
	{
		debug_printf ("Pulse counter: 0x%08X\n", pulse_count);
		GPIOSetValue (LED_PORT, LED_PIN0, LED_ON);
		pmu_wait_ms(10);
		GPIOSetValue (LED_PORT, LED_PIN0, LED_OFF);
		pmu_wait_ms(90);
	}

	return 0;
}
