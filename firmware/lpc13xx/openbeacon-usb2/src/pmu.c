/***************************************************************
 *
 * OpenBeacon.org - Power Management
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
#include "pin.h"
#include "pmu.h"

/* Watchdog oscillator control register setup */
#define FREQSEL 1
#define FCLKANA 500000.0
#define DIVSEL 0x1F
#define WDT_OSC_CLK (FCLKANA/(2.0*(1+DIVSEL)))

#define MAINCLKSEL_IRC_OSC 0
#define MAINCLKSEL_SYS_PLL 1
#define MAINCLKSEL_WDT_OSC 2
#define MAINCLKSEL_SYS_PLL_OUT 3

void
deep_sleep_ms (uint32_t milliseconds)
{
  (void) milliseconds;

  /* back up current power states */
  LPC_SYSCON->PDAWAKECFG = LPC_SYSCON->PDRUNCFG;
  __asm volatile ("WFI");
}

void
pmu_sleep (void)
{
}

void
pmu_calibrate (void)
{
}

void
pmu_off (void)
{
  pin_mode_pmu (1);
  SCB->SCR |= NVIC_LP_SLEEPDEEP;
  LPC_PMU->PCON = 0x2;
  __WFI ();
}

void
pmu_init (void)
{
  LPC_SYSCON->PDSLEEPCFG = 0xFFFFFFFF;
}
