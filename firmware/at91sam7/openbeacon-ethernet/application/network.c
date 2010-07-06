/*
	FreeRTOS.org V5.0.0 - Copyright (C) 2003-2008 Richard Barry.

	This file is part of the FreeRTOS.org distribution.

	FreeRTOS.org is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	FreeRTOS.org is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with FreeRTOS.org; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	A special exception to the GPL can be applied should you wish to distribute
	a combined work that includes FreeRTOS.org, without being obliged to provide
	the source code for any proprietary components.  See the licensing section
	of http://www.FreeRTOS.org for full details of how and when the exception
	can be applied.
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
#include <rnd.h>

#include "env.h"
#include "network.h"
#include "SAM7_EMAC.h"
#include "proto.h"
#include "debug_printf.h"

/* lwIP includes. */
#include "lwip/ip.h"
#include "lwip/init.h"
#include "lwip/memp.h"
#include "lwip/stats.h"
#include "lwip/dhcp.h"
#include "netif/loopif.h"

/*------------------------------------------------------------*/
extern err_t ethernetif_init (struct netif *netif);
/*------------------------------------------------------------*/
struct netif EMAC_if;
/*------------------------------------------------------------*/
static struct ip_addr xIpAddr, xNetMask, xGateway, xServer;
/*------------------------------------------------------------*/

static inline const char *
vNetworkNTOA (struct ip_addr ip)
{
  struct in_addr ina;
  ina.s_addr = ip.addr;
  return inet_ntoa (ina);
}

static inline const char *
vNetworkConfigName (int i)
{
  switch (i)
    {
    case IP_AUTOCONFIG_STATIC_IP:
      return "static IP";
      break;
    case IP_AUTOCONFIG_READER_ID:
      return "IP by reader ID";
      break;
    case IP_AUTOCONFIG_DHCP:
      return "DHCP IP";
      break;
    default:
      return "Unknown IP";
    }
}

void
vNetworkDumpHex (const char *data, unsigned int length)
{
  unsigned char c;

  while (length--)
    {
      c = (unsigned char)(*data++);
      debug_printf ("%X%X%s", (c >> 4) & 0xF, c & 0xF,length ? ":":"\n\r");
    }
}

void
vNetworkDumpConfig (void)
{
  debug_printf ("\n\rNetwork Configuration:\n\r"
		"\t%s configuration [%i]\n\r"
		"\tMAC=",
		vNetworkConfigName (env.e.ip_autoconfig),
		env.e.ip_autoconfig);
  
  vNetworkDumpHex (cMACAddress, sizeof (cMACAddress));

  debug_printf ("\tIP      = %s\n\r", vNetworkNTOA (xIpAddr));
  debug_printf ("\tNetmask = %s\n\r", vNetworkNTOA (xNetMask));
  debug_printf ("\tGateway = %s\n\r", vNetworkNTOA (xGateway));
  debug_printf ("\tServer  = %s\n\r", vNetworkNTOA (xServer));
  debug_printf ("\n\r");
}

static void
vNetworkThread (void *pvParameters)
{
  (void) pvParameters;

  /* Initialize lwIP and its interface layer. */
  lwip_init ();

  /* Execute IP config settings */
  switch (env.e.ip_autoconfig)
    {
    case IP_AUTOCONFIG_STATIC_IP:
      xIpAddr = env.e.ip_host;
      xNetMask = env.e.ip_netmask;
      xGateway = env.e.ip_gateway;
      xServer = env.e.ip_server;
      break;
    case IP_AUTOCONFIG_READER_ID:
      xIpAddr.addr =
	htonl (ntohl (env.e.ip_host.addr & env.e.ip_netmask.addr) +
	       env.e.reader_id);
      xNetMask = env.e.ip_netmask;
      xGateway = env.e.ip_gateway;
      break;
    default:
      //case IP_AUTOCONFIG_DHCP:
      IP4_ADDR (&xIpAddr, 0, 0, 0, 0);
      IP4_ADDR (&xNetMask, 0, 0, 0, 0);
      IP4_ADDR (&xGateway, 0, 0, 0, 0);
      break;
    }

  /* Create and configure the EMAC interface. */
  netif_add (&EMAC_if, &xIpAddr, &xNetMask, &xGateway, NULL,
	     ethernetif_init, ip_input);

  /* make it the default interface */
  netif_set_default (&EMAC_if);

  if (env.e.ip_autoconfig == IP_AUTOCONFIG_DHCP)
    {
      /* dhcp kick-off */
      dhcp_start (&EMAC_if);
      dhcp_fine_tmr ();
      dhcp_coarse_tmr ();
    }

  /* bring it up */
  netif_set_up (&EMAC_if);

  debug_printf ("FreeRTOS based WMCU firmware version %s starting.\n",
		VERSION);

  while (pdTRUE)
    {
      int cnt = 0;

      if (env.e.ip_autoconfig == IP_AUTOCONFIG_DHCP)
	{
	  vTaskDelay (DHCP_FINE_TIMER_MSECS / portTICK_RATE_MS);
	  dhcp_fine_tmr ();
	  cnt += DHCP_FINE_TIMER_MSECS;
	  if (cnt >= DHCP_COARSE_TIMER_SECS * 1000)
	    {
	      dhcp_coarse_tmr ();
	      cnt = 0;
	    }
	}

      vTaskDelay (50 / portTICK_RATE_MS);
      vLedSetGreen (0);
      vTaskDelay (50 / portTICK_RATE_MS);
      vLedSetGreen (1);
    }
}

static xTaskHandle networkTaskHandle = NULL;

/*------------------------------------------------------------*/
void
vNetworkInit (void)
{
  unsigned int mac_l;

  /* If no previous environment exists - create a new, but don't store it */
  env_init ();
  if (!env_load ())
    {
      bzero (&env, sizeof (env));
      env.e.reader_id = DEFAULT_READER_ID;
      IP4_ADDR (&env.e.ip_host, 10, 254, 0, DEFAULT_READER_ID);
      IP4_ADDR (&env.e.ip_netmask, 255, 255, 0, 0);
      IP4_ADDR (&env.e.ip_gateway, 10, 254, 0, 1);
    }

  vRndInit (env.e.reader_id);

  mac_l = (MAC_IAB | (env.e.reader_id & MAC_IAB_MASK));
  cMACAddress[0] = (MAC_OID >> 16) & 0xFF;
  cMACAddress[1] = (MAC_OID >> 8) & 0xFF;
  cMACAddress[2] = (MAC_OID >> 0) & 0xFF;
  cMACAddress[3] = (mac_l >> 16) & 0xFF;
  cMACAddress[4] = (mac_l >> 8) & 0xFF;
  cMACAddress[5] = (mac_l >> 0) & 0xFF;

  /* Create the lwIP task.  This uses the lwIP RTOS abstraction layer. */
  xTaskCreate (vNetworkThread, (signed portCHAR *) "NET",
	       TASK_NET_STACK, NULL, TASK_NET_PRIORITY, &networkTaskHandle);
}
