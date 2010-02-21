/***************************************************************
 *
 * OpenBeacon.org - OpenBeacon link layer protocol
 *
 * Copyright 2007 Milosch Meriac <meriac@openbeacon.de>
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
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <string.h>
#include <board.h>
#include <beacontypes.h>
#include <USB-CDC.h>
#include <sort.h>
#include <led.h>
#include "proto.h"

/**********************************************************************/
#define INT_LEVEL_ADC 4
#define ADC_INTEGRATE1 100
#define ADC_INTEGRATE2 100
/**********************************************************************/

// Global variables
static unsigned short ADC_Buffer[2][ADC_INTEGRATE1];	// Buffer
volatile unsigned int value = 0;

static inline void
DumpUIntToUSB (unsigned int data)
{
  int i = 0;
  unsigned char buffer[10], *p = &buffer[sizeof (buffer)];

  do
    {
      *--p = '0' + (unsigned char) (data % 10);
      data /= 10;
      i++;
    }
  while (data);

  while (i--)
    vUSBSendByte (*p++);
}

static void
vnRFtaskRating (void *parameter)
{
  (void) parameter;

  for (;;)
    {
      led_set_green (1);
      vTaskDelay (250 / portTICK_RATE_MS);
      led_set_green (0);
      vTaskDelay (250 / portTICK_RATE_MS);

      DumpUIntToUSB(value);
      vUSBSendByte ('\n');
      vUSBSendByte ('\r');
    }
}

static void
adc_isr_handler (void)
{
  unsigned int count, val;
  static unsigned int ADC_Buffer_Pos = 0, ADC_Integrate_Val = 0, ADC_Integrate_Count = 0;
  unsigned short *p;

  led_set_red (1);

  // Setup DMA and clear ENDRX flag
  p = ADC_Buffer[ADC_Buffer_Pos];
  AT91F_PDC_SetNextRx (AT91C_BASE_PDC_ADC, (unsigned char *) p, ADC_INTEGRATE1);
  ADC_Buffer_Pos = ADC_Buffer_Pos ? 0 : 1;

  count = ADC_INTEGRATE1;
  val=0;
  while (count--)
    val += (*p++) & 0x3FF;

  if(ADC_Integrate_Count++<ADC_INTEGRATE2)
    ADC_Integrate_Val+=val;
  else
  {
    value = ADC_Integrate_Val;

    ADC_Integrate_Val=0;
    ADC_Integrate_Count=0;
  }

  AT91C_BASE_AIC->AIC_EOICR = 0;

  led_set_red (0);
}

static void __attribute__ ((naked)) adc_isr (void)
{
  portSAVE_CONTEXT ();
  adc_isr_handler ();
  portRESTORE_CONTEXT ();
}

void
vInitProtocolLayer (void)
{
  /* Enable Peripherals */
  AT91F_PIO_CfgPeriph (ADC_CLOCK_PIO, 0, ADC_CLOCK_PIN);

  /* Enable ADC */
  AT91F_ADC_CfgPMC ();
  AT91C_BASE_ADC->ADC_MR =
    AT91C_ADC_TRGEN_EN		|
    AT91C_ADC_TRGSEL_TIOA0	|
    AT91C_ADC_LOWRES_10_BIT	|
    AT91C_ADC_SLEEP_NORMAL_MODE |
    (4 << 8) 			| // PRESCAL
    AT91C_ADC_STARTUP		| // STARTUP
    (5 << 24)			; // SHTIM;
  AT91C_BASE_ADC->ADC_CHER = AT91C_ADC_CH4;

  // Setup DMA
  AT91F_PDC_SetRx (AT91C_BASE_PDC_ADC, (unsigned char *) ADC_Buffer[0], ADC_INTEGRATE1);
  AT91F_PDC_SetNextRx (AT91C_BASE_PDC_ADC, (unsigned char *) ADC_Buffer[1], ADC_INTEGRATE1);
  AT91F_PDC_EnableRx (AT91C_BASE_PDC_ADC);

  // Setup interrupts
  AT91F_AIC_ConfigureIt (AT91C_ID_ADC, INT_LEVEL_ADC,
			 AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, adc_isr);
  //  IRQ enable, end of receive buffer
  AT91C_BASE_ADC->ADC_IER = AT91C_ADC_ENDRX;
  // Enable interrupt in AIC
  AT91C_BASE_AIC->AIC_IECR = 1 << AT91C_ID_ADC;

  /* Configure Timer/Counter 0 */
  AT91F_TC0_CfgPMC ();
  AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKDIS;
  AT91C_BASE_TC0->TC_IDR = 0xFF;
  AT91C_BASE_TC0->TC_RC = (MCK / 4) / ADC_CLOCK_FREQUENCY;
  AT91C_BASE_TC0->TC_CMR =
    AT91C_TC_CLKS_TIMER_DIV1_CLOCK |
    AT91C_TC_WAVE | AT91C_TC_WAVESEL_UP_AUTO | AT91C_TC_ACPC_TOGGLE;
  AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKEN;
  AT91C_BASE_TCB->TCB_BCR = AT91C_TCB_SYNC;

  xTaskCreate (vnRFtaskRating, (signed portCHAR *) "nRF_Rating", 128,
	       NULL, TASK_NRF_PRIORITY, NULL);
}
