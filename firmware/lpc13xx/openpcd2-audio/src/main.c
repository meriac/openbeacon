/***************************************************************
 *
 * OpenBeacon.org - main file for OpenPCD2 basic demo
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
#include "storage.h"
#include "audio.h"
#include "lcd.h"

#define LPC_AD_CHANNELS 2
#define LPC_AD_BITS 10
#define LPC_AD_RANGE (1UL<<LPC_AD_BITS)
#define LPC_AD_VREF_mV 3013

#define LPC_VOLTAGE_DIVIDER_L 1300UL
#define LPC_VOLTAGE_DIVIDER_H 470UL
#define LPCVOLTAGE_SCALE(raw) ((raw)*(LPC_VOLTAGE_DIVIDER_H+LPC_VOLTAGE_DIVIDER_L))/LPC_VOLTAGE_DIVIDER_L

#define CHARGER_MIN_mV 3000
#define CHARGER_MAX_mV 4098

#define LPC_ADC_CR ((16UL-1)<<8)
#define LPC_ADC_CR_AD(channel) ((1UL<<(channel))|LPC_ADC_CR|(1UL<<24))

#define LPC_ADC_VALUE_mV(increments) ((LPC_AD_VREF_mV*(increments))/LPC_AD_RANGE)

#define MIFARE_KEY_SIZE 6
const unsigned char mifare_key[MIFARE_KEY_SIZE] =
	{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static void
rfid_hexdump (const void *buffer, int size)
{
	int i;
	const unsigned char *p = (unsigned char *) buffer;

	for (i = 0; i < size; i++)
	{
		if (i && ((i & 3) == 0))
			debug_printf (" ");
		debug_printf (" %02X", *p++);
	}
	debug_printf (" [size=%02i]\n", size);
}

static void
loop_rfid (void)
{
	int res, present, i;
	static unsigned char data[80];

	/* fully initialized */
	GPIOSetValue (LED1_PORT, LED1_BIT, LED_ON);

	/* read firmware revision */
	debug_printf ("\nreading firmware version...\n");
	data[0] = PN532_CMD_GetFirmwareVersion;
	while ((res = rfid_execute (&data, 1, sizeof (data))) < 0)
	{
		debug_printf ("Reading Firmware Version Error [%i]\n", res);
		pmu_wait_ms (450);
		GPIOSetValue (LED1_PORT, LED1_BIT, LED_ON);
		pmu_wait_ms (10);
		GPIOSetValue (LED1_PORT, LED1_BIT, LED_OFF);
	}

	debug_printf ("PN532 Firmware Version: ");
	if (data[1] != 0x32)
		rfid_hexdump (&data[1], data[0]);
	else
		debug_printf ("v%i.%i\n", data[2], data[3]);

	/* enable debug output */
	GPIOSetValue (LED1_PORT, LED1_BIT, LED_ON);
	i = present = 0;
	while (1)
	{
		/* reset upon plugging USB cable */
		if(GPIOGetValue(VUSB_PORT,VUSB_PIN))
			NVIC_SystemReset ();

		/* wait 1s */
		pmu_wait_ms (10);

		/* detect cards in field */
		data[0] = PN532_CMD_InListPassiveTarget;
		data[1] = 0x01;															/* MaxTg - maximum cards    */
		data[2] = 0x00;															/* BrTy - 106 kbps type A   */
		if (((res = rfid_execute (&data, 3, sizeof (data))) >= 11)
			&& (data[1] == 0x01) && (data[2] == 0x01))
		{
			i = 'A'+(icrc16 (&data[7], data[6]) % 26);
			debug_printf ("CARD_ID: %c\n",(char)i);
			if(i!=present)
			{
				/* blink LED to indicate card */
				GPIOSetValue (LED1_PORT, LED1_BIT, LED_ON);
				lcd_update_screen (i);

				GPIOSetValue(LCD_PWRPWM_PORT, LCD_PWRPWM_PIN, 1);
				audio_play (i);
				GPIOSetValue(LCD_PWRPWM_PORT, LCD_PWRPWM_PIN, 0);

				present = i;
				GPIOSetValue (LED1_PORT, LED1_BIT, LED_OFF);
			}
		}
		else
		{
			GPIOSetValue (LED1_PORT, LED1_BIT, LED_ON);
			if (res != -8)
				debug_printf ("PN532 error res=%i\n", res);
		}

		/* turning field off */
		data[0] = PN532_CMD_RFConfiguration;
		data[1] = 0x01;															/* CfgItem = 0x01           */
		data[2] = 0x00;															/* RF Field = off           */
		rfid_execute (&data, 3, sizeof (data));
	}
}

static void
spell (const char* string)
{
	uint8_t data;

	debug_printf("\n\n");

	GPIOSetValue(LCD_PWRPWM_PORT, LCD_PWRPWM_PIN, 1);
	while((data = (uint8_t)(*string++))!=0)
	{
		if(data == ' ')
			pmu_wait_ms (500);
		else
		{
			lcd_update_screen (data);
			audio_play (data);
		}
	}
	GPIOSetValue(LCD_PWRPWM_PORT, LCD_PWRPWM_PIN, 0);
}

static int
charged (void)
{
	int mV;

	/* start AD conversion */
	LPC_ADC->CR = LPC_ADC_CR_AD(4);
	/* wait a ms */
	pmu_wait_ms (1);
	/* read AD conversion */
	mV = LPCVOLTAGE_SCALE(LPC_ADC_VALUE_mV((LPC_ADC->DR4>>6)&0x3FF));

	/* cap on 4100mV max */
	if(mV>4100)
		mV=4100;

	return (100*((mV<=CHARGER_MIN_mV)?0:(mV-CHARGER_MIN_mV)))/(CHARGER_MAX_mV-CHARGER_MIN_mV);
}

int
get_buttons_any (void)
{
	return (GPIOGetValue(2,0) && GPIOGetValue(0,1) && GPIOGetValue(1,4))?0:1;
}

int
get_buttons_all (void)
{
	return (GPIOGetValue(2,0) || GPIOGetValue(0,1) || GPIOGetValue(1,4))?0:1;
}

int
main (void)
{
	int i;

	/* Initialize GPIO (sets up clock) */
	GPIOInit ();

	/* switch PMU to high power mode */
	LPC_IOCON->PIO0_5 = 1 << 8;
	GPIOSetDir (0, 5, 1); //OUT
	GPIOSetValue (0, 5, 0);

	/* Set LED port pin to output */
	GPIOSetDir (LED1_PORT, LED1_BIT, 1);
	GPIOSetValue (LED1_PORT, LED1_BIT, LED_OFF);

	/* Init Power Management Routines */
	pmu_init ();
	pmu_wait_ms (100);

	/* Init SPI */
	spi_init ();

	/* Configure Buttons to input */
	GPIOSetDir (2,0,0);
	GPIOSetDir (0,1,0);
	GPIOSetDir (1,4,0);

	/* Init Battery ADC4 */
	LPC_IOCON->ARM_SWDIO_PIO1_3 = 2;
	LPC_SYSCON->SYSAHBCLKCTRL |= EN_ADC;
	LPC_SYSCON->PDRUNCFG &= ~ADC_PD;

	if(GPIOGetValue(VUSB_PORT,VUSB_PIN))
	{
		storage_init (0x1234);

		/* UART setup */
		UARTInit (115200, 0);

		i=0;
		while(GPIOGetValue(VUSB_PORT,VUSB_PIN))
		{
			if(get_buttons_all())
				storage_erase();

			i=charged();

			debug_printf("Charged [%03i%%] OpenPCD2-Audio v" PROGRAM_VERSION "\n",i);

			if(i>=99)
				GPIOSetValue (LED1_PORT, LED1_BIT, LED_ON);
			else
			{
				GPIOSetValue (LED1_PORT, LED1_BIT, LED_ON);
				pmu_wait_ms (10);
				GPIOSetValue (LED1_PORT, LED1_BIT, LED_OFF);

				pmu_wait_ms (5*(100-i));

				GPIOSetValue (LED1_PORT, LED1_BIT, LED_ON);
				pmu_wait_ms (10);
				GPIOSetValue (LED1_PORT, LED1_BIT, LED_OFF);
			}

			pmu_wait_ms (1000);
		}

		/* reset after unplugging USB cable */
		NVIC_SystemReset ();
	}

	storage_init (0);
	LPC_SYSCON->SYSPLLCTRL = 0x3 | (1<<5);
	SystemCoreClockUpdate ();

	/* UART setup */
	UARTInit (115200, 0);

	/* show storage status */
	storage_status();

	/* Init LCD Display */
	lcd_init ();

	/* Init Audio */
	audio_init ();

	/* Init RFID */
	rfid_init ();

	spell("HELLO");

	/* RUN RFID loop */
	loop_rfid ();
	return 0;
}
