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

int
main (void)
{
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

	/* Init SPI */
	spi_init ();

	/* CDC USB Initialization */
#if 0
	storage_init (0x1234);
#else
	storage_init (0);
	LPC_SYSCON->SYSPLLCTRL = 0x3 | (1<<5);
	SystemCoreClockUpdate ();
#endif

	/* UART setup */
	UARTInit (115200, 0);

	/* Init RFID */
	rfid_init ();

	/* Init LCD Display */
	lcd_init ();

	storage_status ();

#if 0
	storage_erase ();
	while(1);
#endif

	/* Init Audio */
	audio_init ();

	spell("HELLO WORLD");

	/* RUN RFID loop */
	loop_rfid ();

	return 0;
}
