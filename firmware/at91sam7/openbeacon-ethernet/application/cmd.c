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
#include <crc32.h>
#include "env.h"
#include "cmd.h"
#include "network.h"
#include "debug_printf.h"

static int led_setting = 0;

static inline void
vCmdHelp (void)
{
  debug_printf ("\n\nOpenBeacon.org PoE Ethernet II command line help\n"
		"(C) 2010 Milosch Meriac <meriac@bitmanufaktur.de>\n"
		"\t'a' - set reader address ('t10.254.0.100' for 10.254.0.100)\n"
		"\t'b' - boot again\n"
		"\t'c' - show configuration\n"
		"\t'd' - dump chip registers\n"
		"\t'h' - show help\n"
		"\t'i' - set reader id ('i123')\n"
		"\t'l' - red LED ('l[enable=0, disable=1]')\n"
		"\t'm' - netmask config ('m255.255.0.0')\n"
		"\t'n' - network config ('a[static_ip=0, reader_id=1, dhcp=2]')\n"
		"\t'o' - restore original network settings\n"
		"\t'r' - set router ip\n"
		"\t's' - store configuration\n"
		"\t't' - set target server ip ('t1.2.3.4')\n"
		"\t'u' - reset reader to firmware update mode\n"
		"\n\n");
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
    case 'B':
      debug_printf ("rebooting...\n");
      vTaskDelay (1000 / portTICK_RATE_MS);
      while (1);
      break;
/*    case 'D':
    	res=vCmdDumpRegister();
    	break;*/
    case 'C':
      vNetworkDumpConfig ();
      debug_printf ("System configuration:\n"
		    "\tReader ID:%i\n" "\n", env.e.reader_id);

      break;
    case 'I':
      if (assign)
	{
	  if (env.e.reader_id)
	    debug_printf ("ERROR: reader id already set!\n");
	  else
	    {
	      t = atoiEx (cmdline);
	      if (t > 0 && t < MAC_IAB_MASK)
		env.e.reader_id = t;
	      else
		debug_printf
		  ("error: reader_id needs to be between 1 and %u (used '%s')\n",
		   MAC_IAB_MASK, cmdline);
	    }
	}
      debug_printf ("reader_id=%i\n", env.e.reader_id);
      break;
    case 'L':
      if (assign)
	led_setting = atoiEx (cmdline) > 0;
      debug_printf ("red_led=%i\n", led_setting);
      break;
    case 'N':
      if (assign)
	env.e.ip_autoconfig = atoiEx (cmdline);
      debug_printf ("ip_autoconfig=%i\n", env.e.ip_autoconfig);
      break;
    case 'O':
      /* backup reader id */
      t = env.e.reader_id;
      vNetworkResetDefaultSettings ();
      /* restore reader id */
      env.e.reader_id = t;

      debug_printf ("restoring original settings...\n");
      vNetworkDumpConfig ();
      vTaskDelay (1000 / portTICK_RATE_MS);
      env_store ();
      break;
    case 'R':
      break;
    case 'S':
      debug_printf ("storing configuration & reboot...\n");
      vTaskDelay (1000 / portTICK_RATE_MS);
      env_store ();
      while (1);
      break;
    case 'T':
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
    default:
      debug_printf ("Uknown CMD:'%s'\n", cmdline);
    }
}

static void
vCmdTask (void *pvParameters)
{
  (void) pvParameters;
  portCHAR cByte;
  portBASE_TYPE len = 0;
  static char next_command[MAX_CMD_LINE_LENGTH];

  for (;;)
    {
      if (vUSBRecvByte (&cByte, 1, 100))
	{
	  vUSBSendByte (cByte);
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
    }
}

void
PtCmdInit (void)
{
  xTaskCreate (vCmdTask, (signed portCHAR *) "CMD", TASK_CMD_STACK, NULL,
	       TASK_CMD_PRIORITY, NULL);
}
