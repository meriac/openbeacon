/***************************************************************
 *
 * OpenBeacon.org - main file for OpenBeacon USB II Bluetooth
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

/* ADC0 @ 1MHz clock / 10 bits */
#define ADC_MODE (0x01UL|((((uint32_t)(SYSTEM_CORE_CLOCK/1000000UL))-1)<<8)|(1UL<<16))

#define LED_OFF 0x00
#define LED_R   0x01
#define LED_G   0x02
#define LED_B   0x04

#define COLOR_OVS 1024
#define COLOR_MAX 4

static volatile uint8_t g_rgb_ready;
static volatile uint32_t g_adc_rgba[COLOR_MAX], g_adc_color, g_adc_count;

static inline void set_rgb(uint8_t rgb)
{
	LPC_GPIO1->MASKED_ACCESS[0xE] = rgb << 1;
}

static inline void adc_start(void)
{
	g_rgb_ready = 0;
	g_adc_color = g_adc_count = 0;
	memset(&g_adc_rgba, 0, sizeof(g_adc_rgba));
	LPC_ADC->CR = ADC_MODE|(1<<24);
}

void ADC_IRQHandler(void)
{
	g_adc_count++;
	if(g_adc_count<COLOR_OVS)
		g_adc_rgba[g_adc_color] += (LPC_ADC->GDR >> 6) & 0x3FFUL;
	else
	{
		g_adc_count=0;
		g_adc_color++;

		if(g_adc_color>=COLOR_MAX)
		{
			LPC_ADC->CR = ADC_MODE;
			g_adc_color = 0;
			g_rgb_ready = 1;
		}

		/* update LED */
		set_rgb(g_adc_color ? (1<<(g_adc_color-1)) : LED_OFF);
	}
}

static void init_hardware(void)
{
	/* init wait timer */
	pmu_init();

	/* Initialize GPIO (sets up clock) */
	GPIOInit ();
	GPIOSetDir (BUT1_PORT, BUT1_PIN, 0);
	GPIOSetDir (BUT2_PORT, BUT2_PIN, 0);

	/* initialize LED RGB port */
	LPC_GPIO1->DIR = 0x0E;
	LPC_IOCON->JTAG_TDO_PIO1_1   = 0x81;
	LPC_IOCON->JTAG_nTRST_PIO1_2 = 0x81;
	LPC_IOCON->ARM_SWDIO_PIO1_3  = 0x81;
	set_rgb(LED_OFF);

	/* CDC USB Initialization */
	init_usbserial ();

	/* enabled ADC0 @ 1MHz clock / 10 bits */
	LPC_SYSCON->PDRUNCFG &= ~(1UL<<4);
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 13);
	LPC_IOCON->JTAG_TDI_PIO0_11  = 0x02;
	LPC_ADC->CR = ADC_MODE;
	LPC_ADC->INTEN = 1<<8;
	NVIC_EnableIRQ (ADC_IRQn);
}

int
main (void)
{
	int i, sequence;

	init_hardware();

	sequence = 0;
	while(1)
	{
		/* collect RGB data */
		adc_start();
		while(!g_rgb_ready)
			__WFI();

		/* print RGBA values */
		debug_printf("%08u",sequence++);
		for(i=0; i<COLOR_MAX; i++)
			debug_printf(",%u", g_adc_rgba[i] / COLOR_OVS);
		debug_printf("\n\r");
	}

	return 0;
}
