/* usbshell.c - command line interface for configuration and status inquiry
 *
 * Copyright (c) 2008  The Blinkenlights Crew
 *                          Daniel Mack <daniel@caiaq.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


/* Standard includes. */
#include <stdio.h>
#include <string.h>

/* Scheduler includes. */
#include <FreeRTOS.h>
#include <board.h>
#include <task.h>
#include <semphr.h>
#include <beacontypes.h>
#include "openbeacon.h"
#include "debug_print.h"
#include "env.h"
#include "USB-CDC.h"
#include "dimmer.h"
#include "update.h"
#include "led.h"
#include "usbshell.h"
#include "proto.h"
#include "main.h"

#define PROMPT "\nWDIM> "

#define shell_print(x) DumpStringToUSB(x)

static void
cmd_status (const portCHAR * cmd)
{
  int i;

  shell_print("WDIM state, firmware version " VERSION "\n");
  shell_print("   MAC = 0x");
  DumpHexToUSB(env.e.mac, 2);
  shell_print("\n");

  shell_print ("   LAMP ID = ");
  DumpUIntToUSB (env.e.lamp_id);
  shell_print ("\n");

  shell_print ("   WMCU ID = ");
  DumpUIntToUSB (env.e.wmcu_id);
  shell_print ("\n");

  shell_print ("   current dim value = ");
  DumpUIntToUSB (vGetDimmerStep ());
  shell_print ("\n");
  
  if (vGetDimmerOff())
    shell_print ("   DIMMER IS CURRENTLY FORCED OFF!\n");
  else
    shell_print ("   dimmer not forced off.\n");

  shell_print ("   dimmer jitter = ");
  DumpUIntToUSB ( vGetDimmerJitterUS() );
  shell_print ("\n");

  shell_print ("   EMI pulses = ");
  DumpUIntToUSB ( vGetEmiPulses() );
  shell_print ("\n");
  
  shell_print ("   packet count = ");
  DumpUIntToUSB ( packet_count );
  shell_print ("\n");
  
  shell_print ("   last sequence number = ");
  DumpUIntToUSB ( last_sequence );
  shell_print ("\n");
  
  shell_print ("   pings: last seq ");
  DumpUIntToUSB ( last_ping_seq );
  shell_print (", lost ");
  DumpUIntToUSB ( pings_lost );
  shell_print ("\n");

  shell_print ("   dimmer delay = ");
  DumpUIntToUSB ( env.e.dimmer_delay );
  shell_print (" ms\n");

  shell_print ("   GAMMA table:\t");
  for (i = 0; i < GAMMA_SIZE; i++)
    {
      DumpUIntToUSB (env.e.gamma_table[i]);
      shell_print (" ");

      if (i % 8 == 7)
	shell_print ("\n\t\t");
    }
  shell_print ("\n");
}

static void
cmd_reset (const portCHAR * cmd)
{
  vResetEnv();
  env_store();
}

static void
cmd_help (const portCHAR * cmd)
{
  shell_print ("Blinkenlights command shell help.\n");
  shell_print ("---------------------------------\n");
  shell_print ("\n");
  shell_print ("d[ebug] <level>			Define the debug level\n");
  shell_print ("dim <value>			Set the dimmer to a value (between 0 and 15)\n");
  shell_print ("help				This screen\n");
  shell_print ("id <mcu_id> <lamp_id>		Set mcu and lamp id\n");
  shell_print ("reset				Reset the non-volatile flash to defaults\n");
  shell_print ("mac <xxyy> [<crc>]		Set the MAC address of this unit.\n");
  shell_print ("nrf_dump			dumps 2.4GHz frontend (nRF24L01) register set\n");  
  shell_print ("nrf_init			Initialize 2.4GHz frontend from scratch\n");  
  shell_print ("nrf_reset			reset 2.4GHz frontend FIFOs\n");  
  shell_print ("status				Print status information about this unit.\n");
  shell_print ("update				Enter update mode - DO NOT USE FOR FUN\n");
}

static int
hex_to_int (char *nibble)
{
  if (*nibble >= 'A' && *nibble <= 'F')
    {
      *nibble -= 'A';
      *nibble += 10;
    }
  else if (*nibble >= 'a' && *nibble <= 'f')
    {
      *nibble -= 'a';
      *nibble += 10;
    }
  else if (*nibble >= '0' && *nibble <= '9')
    *nibble -= '0';
  else
    return -1;

  return 0;
}

static void
cmd_mac (const portCHAR * cmd)
{
  portCHAR buf[4], mac_l, mac_h;
  unsigned int i;

  while (*cmd && *cmd != ' ')
    cmd++;

  cmd++;

  for (i = 0; i < sizeof (buf); i++)
    {
      if (!*cmd)
	{
	  shell_print ("bogus command.\n");
	  return;
	}

      buf[i] = *cmd++;
    }

  if (hex_to_int (buf + 0) < 0 ||
      hex_to_int (buf + 1) < 0 ||
      hex_to_int (buf + 2) < 0 || hex_to_int (buf + 3) < 0)
    {
      shell_print ("invalid mac!\n");
      return;
    }

  mac_h = buf[0] << 4 | buf[1];
  mac_l = buf[2] << 4 | buf[3];

  /* checksum given? */
  if (*cmd++ == ' ')
    {
      portCHAR crc;

      buf[0] = *cmd++;
      if (!*cmd)
	{
	  shell_print ("bogus checksum!\n");
	  return;
	}

      buf[1] = *cmd++;
      hex_to_int (buf + 0);
      hex_to_int (buf + 1);

      crc = buf[0] << 4 | buf[1];

      if (crc != (mac_l ^ mac_h))
	{
	  shell_print ("invalid checksum - command ignored\n");
	  return;
	}
    }

  shell_print ("setting new MAC.\n");
  shell_print ("Please power-cycle the device to make"
	       " this change take place.\n");

  /* set it ... */
  env.e.mac = (mac_h << 8) | mac_l;
  env_store ();
}

static void
cmd_dim (const portCHAR * cmd)
{
  int val = 0;

  while (*cmd && *cmd != ' ')
    cmd++;

  cmd++;

  while (*cmd >= '0' && *cmd <= '9')
    {
      val *= 10;
      val += *cmd - '0';
      cmd++;
    }

  vUpdateDimmer (val);
  shell_print ("setting dimmer to value ");
  DumpUIntToUSB (vGetDimmerStep ());
  shell_print ("\n");
}

static void
cmd_id (const portCHAR * cmd)
{
  int mcu_id = 0, lamp_id = 0;

  while (*cmd && *cmd != ' ')
    cmd++;

  cmd++;

  while (*cmd >= '0' && *cmd <= '9')
    {
      mcu_id *= 10;
      mcu_id += *cmd - '0';
      cmd++;
    }

  if (*cmd != ' ')
    {
      shell_print ("parameter missing\n");
      return;
    }

  cmd++;

  while (*cmd >= '0' && *cmd <= '9')
    {
      lamp_id *= 10;
      lamp_id += *cmd - '0';
      cmd++;
    }

  shell_print ("setting wmcu id ");
  DumpUIntToUSB (mcu_id);
  shell_print (", lamp id ");
  DumpUIntToUSB (lamp_id);
  shell_print ("\n");

  if (mcu_id != env.e.wmcu_id ||
      lamp_id != env.e.lamp_id)
    {
      env.e.wmcu_id = mcu_id;
      env.e.lamp_id = lamp_id;
      PtUpdateWmcuId ( env.e.wmcu_id );
      vTaskDelay (100 / portTICK_RATE_MS);
      shell_print ("storing.\n");
      env_store();
    }
  else
    {
      shell_print ("not storing, values are the same.\n");
    }
}

static void
cmd_debug (const portCHAR * cmd)
{
  int val = 0;

  while (*cmd && *cmd != ' ')
    cmd++;

  cmd++;

  if (*cmd < '0' || *cmd > '9')
    return;
  
  val = *cmd - '0';

  shell_print ("setting debug level to ");
  DumpUIntToUSB(val);
  shell_print ("\n");
  debug = val;
}

void
cmd_update (const portCHAR * cmd)
{
  DeviceRevertToUpdateMode ();
}

void
cmd_nrf_dump(const portCHAR * cmd)
{
  PtDumpNrfRegisters();
}

void
cmd_nrf_init(const portCHAR * cmd)
{
  PtInitNrfFrontend(PTINITNRFFRONTEND_INIT);
}

void
cmd_nrf_reset(const portCHAR * cmd)
{
  PtInitNrfFrontend(PTINITNRFFRONTEND_RESETFIFO);
}

static struct cmd_t
{
  const portCHAR *command;
  void (*callback) (const portCHAR * cmd);
} commands[] =
{
  { "debug",		&cmd_debug	},
  { "dim",		&cmd_dim	},
  { "help", 		&cmd_help	},
  { "id",		&cmd_id		},
  { "nrf_dump",		&cmd_nrf_dump	},
  { "nrf_init",		&cmd_nrf_init	},
  { "nrf_reset",	&cmd_nrf_reset	},
  { "reset",		&cmd_reset	},
  { "status", 		&cmd_status	},
  { "update",		&cmd_update	},
  { "mac", 		&cmd_mac 	},
  { "MAC", 		&cmd_mac 	},
    /* end marker */
  { NULL, NULL }
};

static void
parse_cmd (const portCHAR * cmd)
{
  struct cmd_t *c;

  if (strlen (cmd) == 0)
    return;

  shell_print ("\n");
  for (c = commands; c && c->command && c->callback; c++)
    {
      if (strncmp (cmd, c->command, strlen (c->command)) == 0 && c->callback)
	{
	  c->callback (cmd);
	  return;
	}
    }

  shell_print ("unknown command '");
  shell_print (cmd);
  shell_print ("'\n");
}

static void
usbshell_task (void *pvParameters)
{
  static portCHAR buf[128], c, *p = buf;
  shell_print (PROMPT);

  for (;;)
    {
      if (!vUSBRecvByte (&c, sizeof (c), 100))
	continue;

      if (c == '\n' || c == '\r')
	{
	  *p = '\0';
	  parse_cmd (buf);

	  p = buf;
	  shell_print (PROMPT);
	  continue;
	}
      else if (c < ' ')
	continue;

      if (p == buf + sizeof (buf) - 1)
	{
	  p = buf;
	  *p = '\0';
	}

      /* local echo */
      vUSBSendByte (c);
      *p++ = c;
    }
}

void
vUSBShellInit (void)
{
  xTaskCreate (usbshell_task, (signed portCHAR *) "USBSHELL",
	       TASK_USBSHELL_STACK, NULL, TASK_USBSHELL_PRIORITY, NULL);
}
