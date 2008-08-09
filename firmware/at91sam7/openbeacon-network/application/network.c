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

/* lwIP includes. */
#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "lwip/memp.h"
#include "lwip/stats.h"
#include "lwip/dhcp.h"
#include "netif/loopif.h"
#include "env.h"
/*------------------------------------------------------------*/
extern err_t ethernetif_init (struct netif *netif);
/*------------------------------------------------------------*/

/*------------------------------------------------------------*/
void
vNetworkThread (void *pvParameters)
{
  (void) pvParameters;
  static struct netif EMAC_if;
  static struct ip_addr xIpAddr, xNetMask, xGateway;
  
  /* Initialize lwIP and its interface layer. */
  tcpip_init ( NULL,NULL );

  /* Create and configure the EMAC interface. */
  IP4_ADDR (&xIpAddr, emacIPADDR0, emacIPADDR1, emacIPADDR2, emacIPADDR3);
  IP4_ADDR (&xNetMask, emacNET_MASK0, emacNET_MASK1, emacNET_MASK2,
	    emacNET_MASK3);
  IP4_ADDR (&xGateway, emacGATEWAY_ADDR0, emacGATEWAY_ADDR1,
	    emacGATEWAY_ADDR2, emacGATEWAY_ADDR3);
  netif_add (&EMAC_if, &xIpAddr, &xNetMask, &xGateway, NULL, ethernetif_init,
	     tcpip_input);

  /* make it the default interface */
  netif_set_default (&EMAC_if);

  /* dhcp kick-off */
/*  dhcp_coarse_tmr ();
  dhcp_fine_tmr ();
  dhcp_start (&EMAC_if);*/
  
  /* bring it up */
  netif_set_up (&EMAC_if);

  debug_printf ("FreeRTOS based WMCU firmware version %s starting.\n",
		VERSION);
		
  vLedSetGreen (1);

  while (pdTRUE)
    {
	vTaskDelay ( 100 / portTICK_RATE_MS );
    }
}

/*------------------------------------------------------------*/
void
vNetworkInit (void)
{
  /* Create the lwIP task.  This uses the lwIP RTOS abstraction layer. */
  xTaskCreate (vNetworkThread, (signed portCHAR *) "NET",
	       TASK_NET_STACK, NULL, TASK_NET_PRIORITY, NULL);
}
