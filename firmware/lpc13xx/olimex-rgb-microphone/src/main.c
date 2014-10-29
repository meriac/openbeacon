/***************************************************************
 *
 * OpenBeacon.org - RGB Strip control
 *
 * Copyright 2014 Milosch Meriac <milosch@meriac.com>
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
#include "cie1931.h"

#define FFT_SIZE 256
#define FFT_SIZE2 (FFT_SIZE/2)
#define CIE_MAX_INDEX2 (CIE_MAX_INDEX/2)
#define SPI_CS_RGB SPI_CS(LED_PORT,LED_PIN1, 6, SPI_CS_MODE_NORMAL )

#define VOLUME_SLEW_RATE 0.01f;
#define DISPLAY_FREQ_LOW_INDEX 5
#define DISPLAY_FREQ_HIGH_INDEX 120
#define DISPLAY_FREQ_STEPS 64
#define COLOUR_MAX (CIE_MAX_INDEX-1)

/* ADC0 @ 10 bits */
#define ADC_DIVIDER 0xFF
#define FFT_SAMPLING_RATE ((int)((SYSTEM_CORE_CLOCK/(ADC_DIVIDER+1))/11))
#define OVERSAMPLING 4
#define ADC_MODE (0x01UL|(ADC_DIVIDER<<8)|(1UL<<16))

static volatile BOOL g_done;
static uint16_t g_buffer_pos;
static int g_oversampling, g_oversampling_count;
static uint8_t g_led[FFT_SIZE][3];
static int16_t g_buffer[FFT_SIZE];
static float32_t g_samples[FFT_SIZE][2];
static float32_t g_fft[FFT_SIZE];
static float32_t g_window[FFT_SIZE];

void update_leds(void)
{
	/* transmit new values */
	spi_txrx (SPI_CS_RGB, &g_led, sizeof(g_led), NULL, 0);

	/* latch data */
	memset(&g_led, 0, sizeof(g_led));
	spi_txrx (SPI_CS_RGB, &g_led, sizeof(g_led), NULL, 0);
}

void ADC_IRQHandler(void)
{
	if(g_oversampling_count < OVERSAMPLING)
	{
		/* oversample data */
		g_oversampling += (LPC_ADC->GDR >> 6) & 0x3FFUL;
		g_oversampling_count++;
	}
	else
	{
		if(g_buffer_pos<FFT_SIZE)
			g_buffer[g_buffer_pos++] = g_oversampling - (OVERSAMPLING*512);
		else
		{
			/* stop ADC */
			LPC_ADC->CR = ADC_MODE;
			g_done = TRUE;
		}

		g_oversampling_count = g_oversampling = 0;
	}
}

static void adc_start(void)
{
	g_done = FALSE;
	for (int i = 0; i < FFT_SIZE2; i++)
		g_buffer[i] = g_buffer[i + FFT_SIZE2];
	g_buffer_pos = FFT_SIZE2;
	g_oversampling = g_oversampling_count = 0;
	LPC_ADC->CR = ADC_MODE|(1<<24);
}

static void adc_init(void)
{
	/* enabled ADC0 @ 1MHz clock / 10 bits */
	LPC_SYSCON->PDRUNCFG &= ~(1UL<<4);
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 13);
	LPC_IOCON->JTAG_TDI_PIO0_11  = 0x02;
	LPC_ADC->CR = ADC_MODE;
	LPC_ADC->INTEN = 1<<8;
	NVIC_EnableIRQ (ADC_IRQn);
}

void
CDC_GetCommand (unsigned char *command)
{
	(void) command;
}

int
main (void)
{
	float32_t slowMax = 0;
	float32_t max;
	uint32_t index;
	int i,rgb[3];
	float colour;

	/* Initialize GPIO (sets up clock) */
	GPIOInit ();

	/* Set LED port pin to output */
	GPIOSetDir (LED_PORT, LED_PIN0, 1);
	GPIOSetValue (LED_PORT, LED_PIN0, LED_OFF);

	/* Init Power Management Routines */
	pmu_init ();

	/* CDC USB Initialization */
	init_usbserial ();

	/* setup SPI chipselect pin */
	spi_init ();
	spi_init_pin (SPI_CS_RGB);

	/* initialize ADC & FFT buffers */
	adc_init();
	memset(&g_buffer, 0, sizeof(g_buffer));

	/* start first ADC conversion */
	adc_start();

	/* create canned windowing function */
	for(i=0; i<FFT_SIZE; i++) {
		g_window[i] = 0.5-0.5*cos((i*M_PI*2)/FFT_SIZE);
	}

	/* transmit image */
	while(1)
	{
		/* wait for previous ADC to finish */
		while(!g_done)
			__WFE();

		/* copy previous conversion */
		for(i=0; i<FFT_SIZE; i++)
		{
			g_samples[i][0] = g_buffer[i]*g_window[i];
			g_samples[i][1] = 0;
		}

		/* perform next ADC sampling while FFT'ing */
		adc_start();

		/* perform FFT */
		arm_cfft_f32(&arm_cfft_sR_f32_len256, g_samples[0], 0, 1);
		arm_cmplx_mag_f32(g_samples[0], g_fft, FFT_SIZE2);

		/* perceptual weighting */
		for(i=0; i<FFT_SIZE2; i++)
			g_fft[i]*=i;

		/* find maximum */
		arm_max_f32(g_fft, FFT_SIZE2, &max, &index);

		if(max<1)
			continue;
		slowMax += (max - slowMax)*VOLUME_SLEW_RATE;

		float freqStep = expf(logf(DISPLAY_FREQ_HIGH_INDEX/(float)DISPLAY_FREQ_LOW_INDEX)/(DISPLAY_FREQ_STEPS + 1));
		float freqIndexLow = DISPLAY_FREQ_LOW_INDEX;
		float freqIndexHigh = freqIndexLow*freqStep;

		for(i=0; i<DISPLAY_FREQ_STEPS; i++)
		{
			/* average over an appropriate range */
			int lowIndex = (int)freqIndexLow;
			int highIndex = (int)(freqIndexHigh + 1);
			float fftTotal = 0;
			for (int j = lowIndex; j < highIndex; j++) {
				fftTotal += g_fft[j];
			}
			
			/* normalize */
			colour = fftTotal/(highIndex - lowIndex)/slowMax;
			if (colour > 1)
				colour = 1;

			rgb[0] = colour*COLOUR_MAX;
			rgb[1] = sqrtf(colour)*(1 - colour*colour)*COLOUR_MAX;
			rgb[2] = 0.15f*(1 - colour)*COLOUR_MAX;

			g_led[i][1] = g_cie[rgb[0]];
			g_led[i][2] = g_cie[rgb[1]];
			g_led[i][0] = g_cie[rgb[2]];
			
			freqIndexLow = freqIndexHigh;
			freqIndexHigh *= freqStep;
		}

		/* send data */
		update_leds();

		/* blink LED */
		GPIOSetValue (LED_PORT, LED_PIN0, LED_ON);
		pmu_wait_ms(2);
		GPIOSetValue (LED_PORT, LED_PIN0, LED_OFF);
	}
}
