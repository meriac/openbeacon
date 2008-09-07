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
#define MAX_TELEGRAM_LEN 16
struct telegram
{
  int len;
  unsigned char data[MAX_TELEGRAM_LEN];
};

const struct telegram TELEGRAM_OPEN = {
  8,
  {0x02, 0x12, 0x34, 0x56,
   0x00, 0x00, 0x5c, 0x00,},
};

static struct
{
  enum
  { STOPPED, STARTING, RUNNING, SUCCESS } state;
  const struct telegram *telegram;
  int byte, bit, half_bit;
} interrupt_state;

#define TICKS_PER_500_MSECOND			11981

xSemaphoreHandle waitSemaphore;

/**********************************************************************/

static void vWriteCylinderLine (int status);

/**********************************************************************/

static void
stopTimer (void)
{
  AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKDIS;
}

static int
sendTelegramBlocking (const struct telegram *telegram)
{
  taskENTER_CRITICAL ();
  if (interrupt_state.state != STOPPED)
    {
      taskEXIT_CRITICAL ();
      return 0;
    }
  stopTimer ();
  interrupt_state.telegram = telegram;
  interrupt_state.byte = 0;
  interrupt_state.bit = 0;
  interrupt_state.half_bit = 0;
  interrupt_state.state = STARTING;
  taskEXIT_CRITICAL ();

  AT91F_AIC_EnableIt (AT91C_ID_TC0);
  AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;	// start counter
  xSemaphoreTake (waitSemaphore, 100 * portTICK_RATE_MS);	// Will wait for the semaphore to be free again
  AT91F_AIC_DisableIt (AT91C_ID_TC0);

  taskENTER_CRITICAL ();
  stopTimer ();
  if (interrupt_state.state != SUCCESS)
    {
      interrupt_state.state = STOPPED;
      taskEXIT_CRITICAL ();
      return 0;
    }
  else
    {
      interrupt_state.state = STOPPED;
      taskEXIT_CRITICAL ();
      return 1;
    }
}

static void
nextTelegramHalfBit (void)
{
  const char bit =
    interrupt_state.telegram->data[interrupt_state.
				   byte] & (1 << (7 - interrupt_state.bit));
  if (!interrupt_state.half_bit)
    {
      /* First half */
      if (bit)
	vWriteCylinderLine (1);
      else
	vWriteCylinderLine (0);
    }
  else
    {
      /* Second half */
      if (bit)
	vWriteCylinderLine (0);
      else
	vWriteCylinderLine (1);
    }

  if (++interrupt_state.half_bit == 2)
    {
      interrupt_state.half_bit = 0;
      if (++interrupt_state.bit == 8)
	{
	  interrupt_state.bit = 0;
	  if (++interrupt_state.byte == interrupt_state.telegram->len)
	    {
	      interrupt_state.state = SUCCESS;
	    }
	}
    }
}

void
vTimerISR (void)
{
  portBASE_TYPE taskWoken = pdFALSE;
  vLedSetGreen (1);
  if (AT91C_BASE_TC0->TC_SR & AT91C_TC_CPCS)
    {
      if (interrupt_state.state == RUNNING)
	{
	  nextTelegramHalfBit ();
	  if (interrupt_state.state == SUCCESS)
	    xSemaphoreGiveFromISR (waitSemaphore, &taskWoken);
	}
      else if (interrupt_state.state == STARTING)
	{
	  interrupt_state.state = RUNNING;
	}
    }
  vLedSetGreen (0);
  AT91F_AIC_AcknowledgeIt ();
  if (taskWoken == pdTRUE)
    portYIELD_FROM_ISR ();
}

void __attribute__ ((naked)) vTimerISR_Handler (void)
{
  portSAVE_CONTEXT ();
  vTimerISR ();
  portRESTORE_CONTEXT ();
}


/**********************************************************************/
/* Write cylinder status */
static void
vWriteCylinderLine (int status)
{
  if (status == 1)
    {
      AT91F_PIO_CfgInput (AT91C_BASE_PIOA, CYLINDER_IO);	/* Line high */
    }
  else
    {
      AT91F_PIO_CfgOutput (AT91C_BASE_PIOA, CYLINDER_IO);	/* Line low */
    }
  return;
}

/**********************************************************************/

/* Send telegram to cylinder */
int
sendCylinderTelegram (int wakeup, const struct telegram *telegram)
{
  if (wakeup)
    {
      /* Wakeup Pulse */
      /* generate logical HIGH */
      vWriteCylinderLine (1);
      /* generate logical LOW */
      vWriteCylinderLine (0);
      vTaskDelay (1);
      /* generate logical HIGH */
      vWriteCylinderLine (1);
      vTaskDelay (57);		// in ms
    }

  return sendTelegramBlocking (telegram);
}

/**********************************************************************/

int
vOpenLock (void)
{
  return sendCylinderTelegram (1, &TELEGRAM_OPEN);
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
			 vTimerISR_Handler);
  AT91F_PMC_EnablePeriphClock (AT91C_BASE_PMC,
			       ((unsigned int) 1 << AT91C_ID_TC0));

  AT91C_BASE_TCB->TCB_BMR = 0;
  AT91C_BASE_TC0->TC_CMR = AT91C_TC_CLKS_TIMER_DIV1_CLOCK	// MCK/2
    | AT91C_TC_BURST_NONE	// The clock is not gated by an external signal
    | AT91C_TC_WAVESEL_UP_AUTO;	// UP mode with automatic trigger on RC Compare
  AT91C_BASE_TC0->TC_RC = TICKS_PER_500_MSECOND;	// TC0 Timer interval 500ms
  AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKDIS;
  AT91C_BASE_TC0->TC_IER = AT91C_TC_CPCS;
  return 0;
}
