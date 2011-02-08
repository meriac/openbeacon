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

static void
deep_sleep_cylcles (uin16_t cycles)
{
  LPC_TMR->MR0 = cycles;
}

void
deep_sleep_ms (uint32_t milliseconds)
{
  /* back up current power states */
  LPC_SYSCON->PDWAKECFG = LPC_SYSCON->PDRUNCFG;
  __asm volatile ("WFI");
}

void
pmu_sleep (void)
{
  /* back up current power states */
  LPC_SYSCON->PDWAKECFG = LPC_SYSCON->PDRUNCFG;

  /* remember main clock setting */
  LPC_PMU->GPREG0 = LPC_SYSCON->MAINCLKSEL;

  /* switch to watchdog timer oscillator */
  LPC_SYSCON->MAINCLKSEL = MAINCLKSEL_WDT_OSC;

  /* go to sleep */
  __asm volatile ("WFI");

}

void
pmu_calibrate (void)
{
}

void
pmu_off (void)
{
}

void
pmu_init (void)
{
  /* in deep sleep only watchdog runs by default */
  LPC_SYSCON->PDSLEEPCFG = 0xFFF & ~PMU_WDTOSC_PD;

  /* watchdog configuration */
  LPC_SYSCON->WDTOSCCTRL = DIVSEL | (FREQSEL<<5);
}
