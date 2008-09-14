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

#include "network.h"
#include "SAM7_EMAC.h"
#include "bprotocol.h"
#include "debug_printf.h"

/* lwIP includes. */
#include "lwip/ip.h"
#include "lwip/init.h"
#include "lwip/memp.h"
#include "lwip/stats.h"
#include "lwip/dhcp.h"
#include "netif/loopif.h"
#include "env.h"
/*------------------------------------------------------------*/
extern err_t ethernetif_init (struct netif *netif);
/*------------------------------------------------------------*/
struct netif EMAC_if;

/*------------------------------------------------------------*/
void
vNetworkThread (void *pvParameters)
{
  (void) pvParameters;
  static struct ip_addr xIpAddr, xNetMask, xGateway;
  
  /* Initialize lwIP and its interface layer. */
  lwip_init ();

  /* Create and configure the EMAC interface. */
  
#if 1
  IP4_ADDR (&xIpAddr , 0, 0, 0 ,0);
  IP4_ADDR (&xNetMask, 0, 0, 0 ,0);
  IP4_ADDR (&xGateway, 0, 0, 0 ,0);
#endif

#if 0
  IP4_ADDR (&xIpAddr , 169, 254, 0x11, 0x22);
  IP4_ADDR (&xNetMask, 255, 255, 0 ,0);
  IP4_ADDR (&xGateway, 192, 168, 5 ,1);
#endif

  netif_add (&EMAC_if, &xIpAddr, &xNetMask, &xGateway, NULL, ethernetif_init,
	     ip_input);

  /* make it the default interface */
  netif_set_default (&EMAC_if);

  /* dhcp kick-off */
#if 1
  dhcp_coarse_tmr ();
  dhcp_fine_tmr ();
  dhcp_start (&EMAC_if);
#endif

  /* bring it up */
  netif_set_up (&EMAC_if);

  debug_printf ("FreeRTOS based WMCU firmware version %s starting.\n",
		VERSION);

  bprotocol_init();
  vLedSetGreen (1);

  while (pdTRUE)
    {
      vTaskDelay ( 100 / portTICK_RATE_MS );
    }
}

static xTaskHandle networkTaskHandle = NULL;

/*------------------------------------------------------------*/
void
vNetworkInit (void)
{
#if 0
  env_init ();
  env_load ();

  if ((env.e.mac_l == 0xff && env.e.mac_h == 0xff) ||
      (env.e.mac_l == 0x00 && env.e.mac_h == 0x00))
    {
      debug_printf ("Mac address not set, skipping network intialization\n");
      debug_printf ("Use the 'mac' command to set it.\n");
      return;
    }

  cMACAddress[4] = env.e.mac_h;
  cMACAddress[5] = env.e.mac_l;
#endif

  cMACAddress[4] = 0x11;
  cMACAddress[5] = 0x22;

  if (networkTaskHandle)
    {
      debug_printf("killing running network task ...\n");
      vTaskDelete (networkTaskHandle);
      networkTaskHandle = NULL;
    }

  /* Create the lwIP task.  This uses the lwIP RTOS abstraction layer. */
  xTaskCreate (vNetworkThread, (signed portCHAR *) "NET",
	       TASK_NET_STACK, NULL, TASK_NET_PRIORITY, &networkTaskHandle);
}
