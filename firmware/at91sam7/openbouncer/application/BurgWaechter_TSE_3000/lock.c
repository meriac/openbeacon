/***************************************************************
 *
 * OpenBeacon.org - OpenBeacon link layer protocol
 *
 * Copyright 2007 Angelo Cuccato <cuccato@web.de>
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

#include <AT91SAM7.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include <USB-CDC.h>
#include <board.h>
#include <beacontypes.h>
#include <led.h>
#include "../cmd/usbio.h"

/**********************************************************************/
#define cylinderTelegramLen			16
int cylinderTelegram[cylinderTelegramLen] = { 0x02, 0x12, 0x34, 0x56,
  0x00, 0x00, 0x5c, 0x00,
  0x02, 0x04, 0x00, 0x00,
  0x00, 0x00, 0x80, 0x00
};

//#define TICKS_PER_50_MSECOND                  0.0005*MCK/2
#define TICKS_PER_500_MSECOND			11981

xSemaphoreHandle waitSemaphore;

/**********************************************************************/

/* Write cylinder status */
void
vWait500ms (void)
{
  xSemaphoreTake(waitSemaphore, 0); // Clear semaphore 
  AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;	// start counter
  AT91F_AIC_ClearIt(AT91C_ID_TC0);
  AT91F_AIC_EnableIt(AT91C_ID_TC0);
  xSemaphoreTake(waitSemaphore, 10*portTICK_RATE_MS); // Will wait for the semaphore to be free again
  vLedSetGreen(0);
  AT91F_AIC_DisableIt (AT91C_ID_TC0);
  AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKDIS;
}

void
vTimerWaitISR (void)
{
  portBASE_TYPE taskWoken = pdFALSE;
  if (AT91C_BASE_TC0->TC_SR & AT91C_TC_CPCS)
    xSemaphoreGiveFromISR (waitSemaphore, &taskWoken);
  vLedSetGreen(1);
  AT91F_AIC_AcknowledgeIt ();
  if (taskWoken == pdTRUE)
    portYIELD_FROM_ISR ();
}

void __attribute__ ((naked)) vTimerWaitISR_Handler (void)
{
  portSAVE_CONTEXT ();
  vTimerWaitISR ();
  portRESTORE_CONTEXT ();
}

/**********************************************************************/
/* Write cylinder status */
void
vWriteCylinderLine (int status)
{
//int i;

  if (status == 1)
    {
      AT91F_PIO_CfgInput (AT91C_BASE_PIOA, CYLINDER_IO);	/* Line high */
    }
  else
    {
      AT91F_PIO_CfgOutput (AT91C_BASE_PIOA, CYLINDER_IO);	/* Line low */
    }
//for( i=0; i<1400; i++);
  vWait500ms ();
  return;
}

/**********************************************************************/

/* Send telegram to cylinder */
void
sendCylinderTelegram (int *telegram, int length)
{
  int i, j = 0;

  /* Wakeup Pulse */
  /* generate logical HIGH */
  vWriteCylinderLine (1);
  /* generate logical LOW */
  vWriteCylinderLine (0);
  vWriteCylinderLine (0);
  /* generate logical HIGH */
  vWriteCylinderLine (1);
  vTaskDelay (60);		// in ms

  /* Telegram */
  for (j = 0; j <= length; j++)
    {
      for (i = 7; i >= 0; i--)
	if ((*telegram >> i) & 1)
	  {
	    /* generate logical HIGH */
	    vWriteCylinderLine (1);
	    /* generate logical LOW */
	    vWriteCylinderLine (0);
	  }
	else
	  {
	    /* generate logical LOW */
	    vWriteCylinderLine (0);
	    /* generate logical HIGH */
	    vWriteCylinderLine (1);
	  }
      telegram++;
    }

  /* generate logical HIGH */
  vWriteCylinderLine (1);
}

/**********************************************************************/

void
vOpenLock (void)
{
  sendCylinderTelegram (cylinderTelegram, cylinderTelegramLen);
}

/**********************************************************************/

portBASE_TYPE
vLockInit (void)
{
  AT91F_PIO_CfgInput (AT91C_BASE_PIOA, CYLINDER_IO);
  AT91F_PIO_ClearOutput (AT91C_BASE_PIOA, CYLINDER_IO);

  vSemaphoreCreateBinary (waitSemaphore);
  if (waitSemaphore == NULL)
    {
      // Stirb.
    }
  if (xSemaphoreTake (waitSemaphore, 1) != pdTRUE)
    {
      // Stirb harder.
    }

  AT91F_AIC_ConfigureIt (AT91C_ID_TC0, 4, AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE,
			 vTimerWaitISR_Handler);
  AT91F_PMC_EnablePeriphClock (AT91C_BASE_PMC,
			       ((unsigned int) 1 << AT91C_ID_TC0));

  AT91C_BASE_TCB->TCB_BMR = 0;
  AT91C_BASE_TC0->TC_CMR = AT91C_TC_CLKS_TIMER_DIV1_CLOCK	// MCK/2
    | AT91C_TC_BURST_NONE	// The clock is not gated by an external signal
    | AT91C_TC_CPCSTOP		// Counter Clock Stopped with RC Compare
    | AT91C_TC_WAVESEL_UP_AUTO;	// UP mode with automatic trigger on RC Compare
  AT91C_BASE_TC0->TC_RC = TICKS_PER_500_MSECOND;	// TC0 Timer interval 500ms
  AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKDIS;
  AT91C_BASE_TC0->TC_IER = AT91C_TC_CPCS;
  return 0;
}
