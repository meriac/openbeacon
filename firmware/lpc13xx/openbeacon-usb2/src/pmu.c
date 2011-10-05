/***************************************************************
 *
 * OpenBeacon.org - LPC13xx Power Management Functions
 *
 * Copyright 2011 Milosch Meriac <meriac@openbeacon.de>
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
#include "spi.h"
#include "nRF_API.h"
#include "nRF_CMD.h"
#include "openbeacon-proto.h"

static uint32_t g_sysahbclkctrl;
static volatile uint8_t g_sleeping;

#define MAINCLKSEL_IRC 0
#define MAINCLKSEL_SYSPLL_IN 1
#define MAINCLKSEL_WDT 2
#define MAINCLKSEL_SYSPLL_OUT 3

#define SYSTEM_TMR16B0_PRESCALER 10000

void
WAKEUP_IRQHandlerPIO0_8 (void)
{
  /* re-trigger match output */
  LPC_TMR16B0->EMR &= ~1;
  /* reset wakeup logic */
  LPC_SYSCON->STARTRSRP0CLR = STARTxPRP0_PIO0_8;
  /* disable deep sleep */
  SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

  /* enable previous clock settings  */
  LPC_SYSCON->SYSAHBCLKCTRL = g_sysahbclkctrl;
  /* select MISO function for PIO0_8 */
  LPC_IOCON->PIO0_8 = 1;
  /* set PMU to high power mode */
  GPIOSetValue (0, 5, 0);
  /* did we awake? */
  g_sleeping = FALSE;
  /* vodoo -NOP */
  __NOP ();
}

void
pmu_cancel_timer (void)
{
  /* stop 16B0 timer */
  LPC_TMR16B0->TCR = 2;
  /* set 16B0 timer to 0 */
  LPC_TMR16B0->TC = 0;

  /* call IRQ handler */
  WAKEUP_IRQHandlerPIO0_8();
}

void
pmu_sleep_ms (uint16_t ms)
{
  if (ms < 2)
    ms = 2;

  /* set PMU to low power mode */
  GPIOSetValue (0, 5, 1);

  /* select CT16B0_MAT0 function for PIO0_8 */
  LPC_IOCON->PIO0_8 = 2;

  /* Turn off all other peripheral dividers FIXME save settings */
  g_sysahbclkctrl = LPC_SYSCON->SYSAHBCLKCTRL;

  /* prepare 16B0 timer */
  LPC_TMR16B0->TCR = 2;
  LPC_TMR16B0->PR = (SYSTEM_CRYSTAL_CLOCK/LPC_SYSCON->SYSAHBCLKDIV)/1000;
  LPC_TMR16B0->EMR = 2 << 4;
  LPC_TMR16B0->MR0 = ms;
  /* enable IRQ, reset and timer stop in MR0 match */
  LPC_TMR16B0->MCR = 7;

  /* prepare sleep */
  LPC_PMU->PCON = (1 << 11) | (1 << 8);
  SCB->SCR = SCB_SCR_SLEEPDEEP_Msk;

  /* save current power settings, power WDT on wake */
  LPC_SYSCON->PDAWAKECFG = LPC_SYSCON->PDRUNCFG;
  /* power watchdog oscillator in deep sleep mode */
  LPC_SYSCON->PDSLEEPCFG = ~SYSOSC_PD & 0xFFF;

  /* shut down unused stuff */
  LPC_SYSCON->SYSAHBCLKCTRL = EN_RAM|EN_FLASHARRAY|EN_CT16B0|EN_CT32B0;

  /* start timer */
  LPC_TMR16B0->TCR = 1;

  /* sleep */
  g_sleeping = TRUE;
  while(g_sleeping)
    __WFI ();
}

void
pmu_init (void)
{
  /* enable IRQ routine for PIO0_8 */
  NVIC_EnableIRQ (WAKEUP_PIO0_8_IRQn);

  /* initialize start logic for PIO0_8 */
  LPC_SYSCON->STARTAPRP0 |= STARTxPRP0_PIO0_8;
  LPC_SYSCON->STARTRSRP0CLR = STARTxPRP0_PIO0_8;
  LPC_SYSCON->STARTERP0 |= STARTxPRP0_PIO0_8;

  /* switch MAINCLKSEL to system PLL input */
  LPC_SYSCON->MAINCLKSEL = MAINCLKSEL_SYSPLL_IN;
  /* push clock change */
  LPC_SYSCON->MAINCLKUEN = 0;
  LPC_SYSCON->MAINCLKUEN = 1;
  while (!(LPC_SYSCON->MAINCLKUEN & 1));
  /* update system clock to 3 MHz */
  LPC_SYSCON->SYSAHBCLKDIV = 4;
  LPC_SYSCON->SSPCLKDIV = LPC_SYSCON->SYSAHBCLKDIV/2;

  /* enable 16B0/32B0 timers */
  LPC_SYSCON->SYSAHBCLKCTRL |= EN_CT16B0|EN_CT32B0;

  /* prepare 32B0 timer */
  LPC_TMR32B0->TCR = 2;
  LPC_TMR32B0->PR = SYSTEM_CRYSTAL_CLOCK/LPC_SYSCON->SYSAHBCLKDIV;
  LPC_TMR32B0->EMR = 0;
  /* start 32B0 timer */
  LPC_TMR32B0->TCR = 1;

  /* disable unused jobs */
  LPC_SYSCON->UARTCLKDIV = 0;
  LPC_SYSCON->USBCLKDIV = 0;
  LPC_SYSCON->WDTCLKDIV = 0;
  LPC_SYSCON->CLKOUTDIV = 0;
  LPC_SYSCON->SYSTICKCLKDIV = 0;

  /* disable unused stuff */
  LPC_SYSCON->PDRUNCFG |= (IRCOUT_PD|IRC_PD|ADC_PD|WDTOSC_PD|SYSPLL_PD|USBPLL_PD|USBPAD_PD);

  /* wait till clock settles */
  pmu_sleep_ms (10);
}
