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
#include <led.h>

#include "debug_printf.h"
#include "network.h"
#include "proto.h"
#include "bprotocol.h"
#include "SAM7_EMAC.h"
#include "env.h"
#include "USB-CDC.h"

/* lwIP includes */
#include "lwip/inet.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"

#define PROMPT "\nWMCU> "

#define shell_printf(x...) debug_printf(x)

static void
cmd_status (const portCHAR * cmd)
{
  struct netif *nic = &EMAC_if;
  unsigned int uptime = (xTaskGetTickCount () / portTICK_RATE_MS) / 1000;

  shell_printf ("WMCU status:\n");
  shell_printf ("	Firmware version: %s\n", VERSION);
  shell_printf ("	Uptime: %03d:%02d:%02d\n",
  		uptime / 3600, (uptime / 60) % 60, uptime % 60);
  shell_printf ("	WMCU ID: %d\n", env.e.mcu_id);
  shell_printf ("	MAC address:	%02x:%02x:%02x:%02x:%02x:%02x\n",
		nic->hwaddr[0], nic->hwaddr[1], nic->hwaddr[2],
		nic->hwaddr[3], nic->hwaddr[4], nic->hwaddr[5]);
  shell_printf ("	IP address:	%d.%d.%d.%d\n",
		ip4_addr1 (&nic->ip_addr),
		ip4_addr2 (&nic->ip_addr),
		ip4_addr3 (&nic->ip_addr), ip4_addr4 (&nic->ip_addr));
  shell_printf ("	Network mask:	%d.%d.%d.%d\n",
		ip4_addr1 (&nic->netmask), ip4_addr2 (&nic->netmask),
		ip4_addr3 (&nic->netmask), ip4_addr4 (&nic->netmask));
  shell_printf ("	Gateway addr:	%d.%d.%d.%d\n", ip4_addr1 (&nic->gw),
		ip4_addr2 (&nic->gw), ip4_addr3 (&nic->gw),
		ip4_addr4 (&nic->gw));
  shell_printf ("	Receive statistics: %d packets total, %d frames, %d setup\n",
     b_rec_total, b_rec_frames, b_rec_setup);
  shell_printf ("	RF statistics: sent %d broadcasts, %d unicasts, received %d\n",
     rf_sent_broadcast, rf_sent_unicast, rf_rec);
  shell_printf ("	jam density: %d (wait time in ms)\n", PtGetRfJamDensity());
  shell_printf ("	assigned lamps: %d\n", env.e.n_lamps);
  shell_printf ("	rf power level: %d\n", PtGetRfPowerLevel());
  debug_printf ("\n");
}

static void
cmd_lampmap (const portCHAR * cmd)
{
  unsigned int i;

  shell_printf ("assigned lamps (%d):\n", env.e.n_lamps);
  shell_printf ("	#lamp MAC	#screen		#x	#y	#last value\n");

  for (i = 0; i < env.e.n_lamps; i++)
    {
      debug_printf ("\t0x%04x\t\t%d\t\t%d\t%d\t%d\n",
		    env.e.lamp_map[i].mac,
		    env.e.lamp_map[i].screen,
		    env.e.lamp_map[i].x,
		    env.e.lamp_map[i].y, last_lamp_val[i]);
    }

  debug_printf ("\n");
}

static void
cmd_help (const portCHAR * cmd)
{
  struct netif *nic = &EMAC_if;

  shell_printf ("Blinkenlights command shell help.\n");
  shell_printf ("---------------------------------\n");
  shell_printf ("\n");
  shell_printf ("help\n");
  shell_printf ("	This screen\n");
  shell_printf ("\n");
  shell_printf ("[wmcu-]mac <xxyy> [<crc>]\n");
  shell_printf ("	Set the MAC address of this unit.\n");
  shell_printf ("	Address xxyy is given in two hexadecimal 8bit numbers with\n");
  shell_printf ("	no separator, crc is optional and is ignored when not given.\n");
  shell_printf ("	When given, it needs to be MAC_L ^ MAC_H, otherwise the\n");
  shell_printf ("	command is rejected. The two values which are set here are\n");
  shell_printf ("	the last two digits only with a unchangable prefix, hence\n");
  shell_printf ("	the full MAC would be %02x:%02x:%02x:%02x:xx:yy.\n",
		nic->hwaddr[0], nic->hwaddr[1], nic->hwaddr[2],
		nic->hwaddr[3]);
  shell_printf ("[wmcu-]id <id>\n");
  shell_printf ("	Set the WMCU ID and store it to the flash memory\n");
  shell_printf ("	This also updates all dimmers configured in the lamp map\n");
  shell_printf ("\n");
  shell_printf ("status\n");
  shell_printf ("	Print status information about this unit. Try it, it's fun.\n");
  shell_printf ("lampmap\n");
  shell_printf ("	Dump the lampmap\n");
  shell_printf ("env\n");
  shell_printf ("	Show variables currently stored in the non-volatile flash memory\n");
  shell_printf ("update\n");
  shell_printf ("        Enter update mode - DO NOT USE FOR FUN\n\n");
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
	  shell_printf ("bogus command.\n");
	  return;
	}

      buf[i] = *cmd++;
      if (hex_to_int (buf + i) < 0)
	{
	  shell_printf ("invalid MAC!\n");
	  return;
	}
    }

  mac_h = buf[0] << 4 | buf[1];
  mac_l = buf[2] << 4 | buf[3];

  /* checksum given? */
  if (*cmd == ' ')
    {
      portCHAR crc;

      buf[0] = *cmd++;
      if (!*cmd)
	{
	  shell_printf ("bogus checksum!\n");
	  return;
	}

      buf[1] = *cmd++;
      hex_to_int (buf + 0);
      hex_to_int (buf + 1);
      crc = buf[0] << 4 | buf[1];

      if (crc != (mac_l ^ mac_h))
	{
	  shell_printf ("invalid checksum - command ignored\n");
	  return;
	}
    }

  shell_printf ("setting new MAC: %02x%02x.\n", mac_h, mac_l);
  shell_printf ("Please power-cycle the device to make this change take place.\n");

  env.e.mac_h = mac_h;
  env.e.mac_l = mac_l;
  vTaskDelay (100 / portTICK_RATE_MS);
  env_store ();
}

static void
cmd_id (const portCHAR * cmd)
{
  unsigned int i, id = 0;

  while (*cmd && *cmd != ' ')
    cmd++;

  cmd++;

  for (;;)
    {
      portCHAR b;

      if (!*cmd)
	break;

      b = *cmd++;
      if (b > '9' || b < '0')
	{
	  shell_printf ("invalid ID!\n");
	  return;
	}

      id *= 10;
      id += (b - '0');
    }

  shell_printf ("setting new WMCU ID: %d\n", id);
  env.e.mcu_id = id;

  for (i = 0; i < env.e.n_lamps; i++)
    {
      LampMap *m = env.e.lamp_map + i;
      shell_printf ("updating dimmer 0x%04x -> ID %d\n", m->mac, i);

      b_set_lamp_id (i, m->mac);
      vTaskDelay (100 / portTICK_RATE_MS);
    }

  vTaskDelay (100 / portTICK_RATE_MS);
  env_store ();
}

static void
cmd_update (const portCHAR * cmd)
{
  shell_printf ("resetting to default bootloader in update mode\n");
  vTaskDelay (500 / portTICK_RATE_MS);

  AT91C_UDP_TRANSCEIVER_ENABLE = AT91C_UDP_TXVDIS;

  /* disable USB */
  AT91C_BASE_PIOA->PIO_SODR = AT91C_PIO_PA16;
  vTaskDelay (500 / portTICK_RATE_MS);

  env_reboot_to_update ();
}

static struct cmd_t
{
  const portCHAR *command;
  void (*callback) (const portCHAR * cmd);
} commands[] =
{
  {"help",	&cmd_help},
  {"id",	&cmd_id},
  {"lampmap",	&cmd_lampmap},
  {"mac",	&cmd_mac},
  {"status",	&cmd_status},
  {"update",	&cmd_update},
  {"wmcu-id",	&cmd_id},
  {"wmcu-mac",	&cmd_mac},
    /* end marker */
  {
  NULL, NULL}
};

static void
parse_cmd (const portCHAR * cmd)
{
  struct cmd_t *c;

  if (strlen (cmd) == 0)
    return;

  shell_printf ("\n");
  for (c = commands; c && c->command && c->callback; c++)
    {
      if (strncmp (cmd, c->command, strlen (c->command)) == 0 && c->callback)
	{
	  c->callback (cmd);
	  return;
	}
    }

  shell_printf ("unknown command '%s'\n", cmd);
}

static void
usbshell_task (void *pvParameters)
{
  static portCHAR buf[128], c, *p = buf;
  shell_printf (PROMPT);

  for (;;)
    {
      if (!vUSBRecvByte (&c, sizeof (c), 100))
	continue;

      if (c == '\n' || c == '\r')
	{
	  *p = '\0';
	  parse_cmd (buf);

	  p = buf;
	  shell_printf (PROMPT);
	  continue;
	}

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
