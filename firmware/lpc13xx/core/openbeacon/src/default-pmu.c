/***************************************************************
 *
 * OpenBeacon.org - LPC13xx Power Management Functions
 *
 * Copyright 2011-2012 Milosch Meriac <meriac@openbeacon.de>
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
#include "default-pmu.h"
#ifndef DISABLE_DEFAULT_PMU

static volatile uint8_t g_sleeping;

#define SYSTEM_TMR16B0_PRESCALER 10000

void
TIMER16_0_IRQHandler (void)
{
	g_sleeping = FALSE;

	/* reset interrupt */
	LPC_TMR16B0->IR = 1;
}

void
pmu_wait_ms (uint16_t ms)
{
	if(ms<1)
		ms=1;

	/* prepare sleep */
	LPC_PMU->PCON = (1 << 11) | (1 << 8);
	SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

	/* start timer */
	LPC_TMR16B0->TC = 0;
	LPC_TMR16B0->MR0 = ms * (SYSTEM_TMR16B0_PRESCALER/1000);
	LPC_TMR16B0->TCR = 1;

	/* sleep */
	g_sleeping = TRUE;

	while (g_sleeping)
		__WFI ();
}

void
pmu_init (void)
{
	/* enable timer clock */
	LPC_SYSCON->SYSAHBCLKCTRL |= EN_CT16B0;

	SystemCoreClockUpdate();

	/* reset 16B0 timer */
	LPC_TMR16B0->TCR = 2;
	LPC_TMR16B0->PR = (SystemCoreClock / LPC_SYSCON->SYSAHBCLKDIV) / SYSTEM_TMR16B0_PRESCALER;
	LPC_TMR16B0->EMR = 0;
	/* enable IRQ, reset and timer stop in MR0 match */
	LPC_TMR16B0->MCR = 7;

	/* enable IRQ routine for TMR16B0 */
	NVIC_EnableIRQ (TIMER_16_0_IRQn);
}

#endif/*DISABLE_DEFAULT_PMU*/
