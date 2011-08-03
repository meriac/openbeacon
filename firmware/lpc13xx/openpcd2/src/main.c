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
#include "rfid.h"
#include "pmu.h"

static
void rfid_hexdump(const void *buffer, int size)
{
	int i;
	const unsigned char *p = (unsigned char *) buffer;

	for (i = 0; i < size; i++)
	{
		if (i && ((i & 3) == 0))
			debug_printf(" ");
		debug_printf(" %02X", *p++);
	}
	debug_printf(" [size=%02i]\n", size);
}

static
int rfid_execute(void *data, unsigned int isize, unsigned int osize)
{
	int res;
	if (rfid_write(data, isize))
	{
		debug_printf("getting result\n");
		res = rfid_read(data, osize);
		if (res > 0)
			rfid_hexdump(data, res);
		else
			debug_printf("error: res=%i\n", res);
	}
	else
	{
		debug_printf("->NACK!\n");
		res = -1;
	}
	return res;
}

static
void loop_rfid(void)
{
	int i;
	static unsigned char data[80];

	/* touch unused Parameter */
	/* release reset line after 400ms */
	pmu_wait_ms( 400 );
	rfid_reset(1);
	/* wait for PN532 to boot */
	pmu_wait_ms( 100 );

	/* read firmware revision */
	debug_printf("\nreading firmware version...\n");
	data[0] = PN532_CMD_GetFirmwareVersion;
	rfid_execute(&data, 1, sizeof(data));

	/* enable debug output */
	debug_printf("\nenabling debug output...\n");
	rfid_write_register (0x6328, 0xFC);
	// select test bus signal
	rfid_write_register (0x6321, 6);
	// select test bus type
	rfid_write_register (0x6322, 0x07);

	while (1)
	{
		/* wait 500ms */
		pmu_wait_ms ( 500 );

		/* detect cards in field */
		GPIOSetValue(LED_PORT, LED_BIT, LED_ON);
		debug_printf("\nchecking for cards...\n");
		data[0] = PN532_CMD_InListPassiveTarget;
		data[1] = 0x01; /* MaxTg - maximum cards    */
		data[2] = 0x00; /* BrTy - 106 kbps type A   */
		if (((i = rfid_execute(&data, 3, sizeof(data))) >= 11) && (data[1]
				== 0x01) && (data[2] == 0x01))
		{
			debug_printf("card id: ");
			rfid_hexdump(&data[7], data[6]);
		}
		else
			debug_printf("unknown response of %i bytes\n", i);
		GPIOSetValue(LED_PORT, LED_BIT, LED_OFF);

		/* turning field off */
		debug_printf("\nturning field off again...\n");
		data[0] = PN532_CMD_RFConfiguration;
		data[1] = 0x01; /* CfgItem = 0x01           */
		data[2] = 0x00; /* RF Field = off           */
		rfid_execute(&data, 3, sizeof(data));
	}
}

int main(void)
{
	/* Initialize GPIO (sets up clock) */
	GPIOInit();

	/* Set LED port pin to output */
	GPIOSetDir(LED_PORT, LED_BIT, LED_ON);

#ifdef  ENABLE_USB_FULLFEATURED
	/* CDC Initialization */
	CDC_Init();
	/* USB Initialization */
	USB_Init();
	/* Connect to USB port */
	USB_Connect(1);
#endif/*ENABLE_USB_FULLFEATURED*/

	/* Init Power Management Routines */
	pmu_init();

	/* UART setup */
	UARTInit(115200, 0);

	/* Init RFID */
	rfid_init();

	/* Update Core Clock */
	SystemCoreClockUpdate ();

	/* RUN RFID loop */
	loop_rfid();

	return 0;
}
