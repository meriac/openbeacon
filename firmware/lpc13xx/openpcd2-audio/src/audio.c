/***************************************************************
 *
 * OpenBeacon.org - LPC13xx Audio Routines
 *
 * Copyright 2012 Milosch Meriac <meriac@openbeacon.de>
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
#include "storage.h"
#include "audio.h"

/* Speaker Audio Pins */
#define SPEAKER_PWM_PORT 1
#define SPEAKER_PWM_PIN 2
#define SPEAKER_SHUTDOWN_PORT 1
#define SPEAKER_SHUTDOWN_PIN 1

#define BUFFER_SIZE 1024
#define OVERSAMPLING 3
#define LOWPASS (OVERSAMPLING*4)

static volatile int g_buf_pos;
static uint8_t buffer[BUFFER_SIZE];
static uint8_t *g_buf,g_data,g_data_prev;
static int g_oversampling;

static uint16_t g_lp_buf[LOWPASS],g_lp_pos;
static uint32_t g_lp;

void
TIMER32_1_IRQHandler (void)
{
	uint16_t data;

	/* interpolate sound sample */
	data = (g_oversampling*g_data)+((OVERSAMPLING-g_oversampling)*g_data_prev);

	/* maintain lowpass filter */
	g_lp-= g_lp_buf[g_lp_pos];
	g_lp+= data;
	g_lp_buf[g_lp_pos++]=data;
	if(g_lp_pos>=LOWPASS)
		g_lp_pos=0;

	LPC_TMR32B1->MR1 = g_lp/LOWPASS;

	g_oversampling++;
	if(g_oversampling>OVERSAMPLING)
	{
		g_oversampling=0;

		if(g_buf_pos>=(BUFFER_SIZE-1))
		{
			g_buf_pos=0;
			g_buf=buffer;
		}
		else
			g_buf_pos++;

		g_data_prev = g_data;
		g_data = *g_buf++;
	}

	// reset IRQ source
	LPC_TMR32B1->IR = 1 << 3;
}

void
audio_init (void)
{
	int i;

	/* enable clocks */
	LPC_SYSCON->SYSAHBCLKCTRL |= (EN_CT32B1 | EN_IOCON);

	/* reconfigure speaker shutdown pin */
	LPC_IOCON->JTAG_TDO_PIO1_1= 0x1 | (1<<6) | (1<<7);
	GPIOSetDir (SPEAKER_SHUTDOWN_PORT, SPEAKER_SHUTDOWN_PIN, 1);
	GPIOSetValue (SPEAKER_SHUTDOWN_PORT, SPEAKER_SHUTDOWN_PIN, 1);

	/* reconfigure speaker pwm pin */
	LPC_IOCON->JTAG_nTRST_PIO1_2= 0x3 | (1<<6) | (1<<7);

	/* run 32 bit timer for speaker generation */
	LPC_TMR32B1->TCR = 2;
	LPC_TMR32B1->TCR = 0;
	LPC_TMR32B1->PWMC = 2;
	LPC_TMR32B1->PR = 0;
	LPC_TMR32B1->EMR = 0;
	LPC_TMR32B1->MR1 = 128*OVERSAMPLING;
	LPC_TMR32B1->MR3 = 256*OVERSAMPLING;

	/* reset buffers */
	g_buf_pos = 0;
	g_oversampling = 0;
	g_data = g_data_prev = 128;
	memset(buffer, 128, sizeof(buffer));
	g_buf = buffer;

	/* reset lowpass */
	for(i=0;i<LOWPASS;i++)
		g_lp_buf[i]=128*OVERSAMPLING;
	g_lp = 128*OVERSAMPLING*LOWPASS;
	g_lp_pos = 0;

	/* reset timer on MR3 match, IRQ */
	LPC_TMR32B1->MCR = 3 << 9;

	/* enable CT32B1_MAT_IRQ */
	NVIC_EnableIRQ (TIMER_32_1_IRQn);

	/* start timer */
	LPC_TMR32B1->TCR = 1;

	while(1)
	{
		while(g_buf_pos<=(BUFFER_SIZE/2))
			__WFI();
		GPIOSetValue (LED1_PORT, LED1_BIT, LED_ON);
		storage_read(i,BUFFER_SIZE/2,buffer);
		GPIOSetValue (LED1_PORT, LED1_BIT, LED_OFF);
		i+=BUFFER_SIZE/2;

		while(g_buf_pos>(BUFFER_SIZE/2))
			__WFI();
		GPIOSetValue (LED1_PORT, LED1_BIT, LED_ON);
		storage_read(i,BUFFER_SIZE/2,&buffer[BUFFER_SIZE/2]);
		GPIOSetValue (LED1_PORT, LED1_BIT, LED_OFF);
		i+=BUFFER_SIZE/2;

		if(i>3724735)
			i=0;
	}
}
