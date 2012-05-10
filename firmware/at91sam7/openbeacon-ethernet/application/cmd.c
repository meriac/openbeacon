/***************************************************************
 *
 * OpenBeacon.org - OpenBeacon command line interface
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
#include <USB-CDC.h>
#include <semphr.h>
#include <task.h>
#include <string.h>
#include <board.h>
#include <led.h>
#include <crc32.h>
#include "env.h"
#include "cmd.h"
#include "proto.h"
#include "network.h"
#include "debug_printf.h"

static inline void
vCmdHelp (void)
{
	debug_printf ("\n\nOpenBeacon.org PoE Ethernet II v" PROGRAM_VERSION "\n"
				  "(C) 2010 Milosch Meriac <meriac@bitmanufaktur.de>\n"
				  "\t'a' - set reader address ('a10.254.0.100' for 10.254.0.100)\n"
				  "\t'b' - boot again\n"
				  "\t'c' - show configuration\n"
				  "\t'd' - enable debug output ('d[disable=0,enable=1]')\n"
				  "\t'f' - enable/disable RF packet duplicate filtering\n"
				  "\t      ('f[disabled=0,enabled=1]')\n"
				  "\t'g' - set gateway ip\n"
				  "\t'h' - show help\n"
				  "\t'i' - set reader id ('i123')\n"
				  "\t'l' - red LED ('l[enable=0, disable=1]')\n"
				  "\t'm' - netmask config ('m255.255.0.0')\n"
				  "\t'n' - network config ('n[static_ip=0, reader_id=1, dhcp=2]')\n"
				  "\t'p' - set target server UDP port ('p2342')\n"
				  "\t'r' - restore original network settings\n"
				  "\t's' - store configuration\n"
				  "\t't' - set target server ip ('t1.2.3.4')\n"
				  "\t'u' - reset reader to firmware update mode\n"
				  "\t'x' - show system statistics\n" "\n\n");

}

static inline void
vCmdDumpStatistics (void)
{
	unsigned int h, m, s;

	s = (xTaskGetTickCount () * portTICK_RATE_MS) / 1000;
	h = s / 3600;
	s %= 3600;
	m = s / 60;
	s %= 60;

	debug_printf ("\nSystem Statistics:\n");
	debug_printf ("\tuptime      = %03uh:%02um:%02us\n", h, m, s);
	debug_printf ("\tfree heap   = %u kByte\n",
				  xPortGetFreeHeapSize () / 1024);
	debug_printf ("\n");
}

static int
atoiEx (const char *nptr)
{
	int sign = 1, i = 0, curval = 0;

	while (nptr[i] == ' ' && nptr[i] != 0)
		i++;

	if (nptr[i] != 0)
	{
		if (nptr[i] == '-')
		{
			sign *= -1;
			i++;
		}
		else if (nptr[i] == '+')
			i++;

		while (nptr[i] != 0 && nptr[i] >= '0' && nptr[i] <= '9')
			curval = curval * 10 + (nptr[i++] - '0');
	}

	return sign * curval;
}

static inline void
vCmdProcess (const char *cmdline)
{
	unsigned char cmd, assign;
	unsigned int t;

	cmd = (unsigned char) (*cmdline++);
	if ((cmd >= 'a') && (cmd <= 'z'))
		cmd -= ('a' - 'A');

	switch (*cmdline)
	{
		case '=':
		case ' ':
			assign = 1;
			cmdline++;
			break;
		case 0:
			assign = 0;
			break;
		default:
			assign = 1;
	}

	switch (cmd)
	{
		case 'A':
			vNetworkSetIP (&env.e.ip_host, assign ? cmdline : NULL, "reader");
			break;

		case 'B':
			debug_printf ("rebooting...\n");
			vTaskDelay (1000 / portTICK_RATE_MS);
			/*force watchdog reset */
			while (1);
			break;

		case 'C':
			vNetworkDumpConfig ();
			debug_printf ("System configuration:\n"
						  "\tFirmware Version: v" PROGRAM_VERSION "\n"
						  "\tReader ID: %i\n"
						  "\tRF packet duplicate filter: %s\n\n",
						  env.e.reader_id,
						  env.e.filter_duplicates ? "ON" : "OFF");
			break;

		case 'D':
			if (assign)
			{
				t = atoiEx (cmdline);
				debug_printf ("setting debug level to %u\n", t);
				PtSetDebugLevel (t);
			}
			else
				debug_printf
					("usage: 'd' - set debug level ('d[disable=0, lowest=1]')\n");
			break;

		case 'F':
			if (assign)
			{
				t = atoiEx (cmdline) ? 1 : 0;
				debug_printf ("%s duplicate filtering\n",
							  t ? "enabled" : "disabled");
				env.e.filter_duplicates = t;
			}
			else
				debug_printf
					("usage: 'f' - set RF packet duplicate filtering\n"
					 "             ('f[disable=0, enable=1]')\n");
			break;

		case 'G':
			vNetworkSetIP (&env.e.ip_gateway, assign ? cmdline : NULL,
						   "gateway");
			break;

		case 'I':
			if (assign)
			{
#ifndef ALLOW_READER_ID_CHANGE
				if (env.e.reader_id)
					debug_printf ("ERROR: reader id already set!\n");
				else
				{
#endif /*ALLOW_READER_ID_CHANGE */
					t = atoiEx (cmdline);
					if (t > 0 && t < MAC_IAB_MASK)
						env.e.reader_id = t;
					else
						debug_printf
							("error: reader_id needs to be between 1 and %u (used '%s')\n",
							 MAC_IAB_MASK, cmdline);
#ifndef ALLOW_READER_ID_CHANGE
				}
#endif /*ALLOW_READER_ID_CHANGE */
			}
			debug_printf ("reader_id=%i\n", env.e.reader_id);
			break;

		case 'L':
			if (assign)
			{
				t = atoiEx (cmdline) > 0;
				debug_printf ("red_led=%i\n", t);
				led_set_red (t);
			}
			else
				debug_printf
					("usage: 'l' - red LED ('l[enable=0, disable=1]')\n");
			break;

		case 'M':
			vNetworkSetIP (&env.e.ip_netmask, assign ? cmdline : NULL,
						   "netmask");
			break;

		case 'N':
			if (assign)
				env.e.ip_autoconfig = atoiEx (cmdline);
			debug_printf ("ip_autoconfig=%i\n", env.e.ip_autoconfig);
			break;

		case 'P':
			if (assign)
				env.e.ip_server_port = atoiEx (cmdline);
			debug_printf ("server UDP port=%i\n", env.e.ip_server_port);
			break;

		case 'R':
			/* backup reader id */
			t = env.e.reader_id;
			vNetworkResetDefaultSettings ();
			/* restore reader id */
			env.e.reader_id = t;
			debug_printf ("restoring original settings & reboot...\n");
			vNetworkDumpConfig ();
			vTaskDelay (1000 / portTICK_RATE_MS);
			env_store ();
			/*force watchdog reset */
			while (1);
			break;

		case 'S':
			debug_printf ("storing configuration & reboot...\n");
			vTaskDelay (1000 / portTICK_RATE_MS);
			env_store ();
			/*force watchdog reset */
			while (1);
			break;

		case 'T':
			vNetworkSetIP (&env.e.ip_server, assign ? cmdline : NULL,
						   "server");
			break;

		case 'U':
			debug_printf ("resetting reader to firmware update mode...\n");
			vTaskDelay (1000 / portTICK_RATE_MS);
			env_reboot_to_update ();
			break;

		case 'H':
		case '?':
			vCmdHelp ();
			break;

		case 'X':
			vCmdDumpStatistics ();
			PtStatusRxTx ();
			break;

		default:
			debug_printf ("Unkown CMD:'%c'\n", cmd);
	}
}

static void
vCmdTask (void *pvParameters)
{
	(void) pvParameters;
	unsigned int t = 0;
	portCHAR cByte;
	portBASE_TYPE len = 0;
	static char next_command[MAX_CMD_LINE_LENGTH];

	for (;;)
	{
		if (vUSBRecvByte (&cByte, 1, 100))
		{
			vDebugSendHook (cByte);
			switch (cByte)
			{
				case '\n':
				case '\r':
					if (len)
					{
						next_command[len] = 0;
						vCmdProcess (next_command);
						len = 0;
					}
					break;
				default:
					next_command[len++] = cByte;
			}
		}
		else if (AT91F_PIO_GetInput (EXT_BUTTON_PIO) & EXT_BUTTON_PIN)
		{
			if (t < 30)
				t++;
			else
			{
				for (t = 0; t < 10; t++)
				{
					vTaskDelay (500 / portTICK_RATE_MS);
					led_set_red (0);
					vTaskDelay (500 / portTICK_RATE_MS);
					led_set_red (1);
				}

				/* backup reader id */
				t = env.e.reader_id;
				vNetworkResetDefaultSettings ();
				/* restore reader id */
				env.e.reader_id = t;

				/* store the configuration */
				env_store ();

				/* wait a bit before the reset */
				debug_printf ("restoring original settings"
							  " after reset pin triggerd & reboot...\n");
				vTaskDelay (1000 / portTICK_RATE_MS);

				/*force watchdog reset */
				while (1);
			}
		}
		else
			t = 0;
	}
}

void
PtCmdInit (void)
{
	/* enable firmware defaults button */
	AT91F_PIO_CfgInputFilter (EXT_BUTTON_PIO, EXT_BUTTON_PIN);
	AT91F_PIO_CfgInput (EXT_BUTTON_PIO, EXT_BUTTON_PIN);

	/* enable power LED */
	led_set_red (1);

	xTaskCreate (vCmdTask, (signed portCHAR *) "CMD", TASK_CMD_STACK, NULL,
				 TASK_CMD_PRIORITY, NULL);
}
