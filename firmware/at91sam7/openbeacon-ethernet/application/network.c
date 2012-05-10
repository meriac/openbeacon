/***************************************************************
 *
 * OpenBeacon.org - network code
 *
 * Copyright 2010 Milosch Meriac <meriac@openbeacon.de>
 *
 * basically starts the USB task, initializes all IO ports
 * and introduces idle application hook to handle the HF traffic
 * from the nRF24L01 chip
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
#include "lwip/udp.h"
#include "netif/loopif.h"

/*------------------------------------------------------------*/
extern err_t ethernetif_init (struct netif *netif);
/*------------------------------------------------------------*/
static struct netif EMAC_if;
static struct udp_pcb *vNetworkSocket = NULL;
static struct pbuf *vNetworkSocketBuf = NULL;
/*------------------------------------------------------------*/

static inline const char *
vNetworkNTOA (struct ip_addr ip)
{
	struct in_addr ina;
	ina.s_addr = ip.addr;
	return inet_ntoa (ina);
}

int
vNetworkSetIP (struct ip_addr *ip, const char *ip_string,
			   const char *ip_class)
{
	int res = 0;
	struct in_addr ina;

	if (!ip)
		return 0;

	if (!ip_class)
		ip_class = "unknown";

	if (ip_string)
	{
		if (inet_aton (ip_string, &ina))
		{
			ip->addr = ina.s_addr;
			debug_printf ("%s IP set to %s\n", ip_class, inet_ntoa (ina));
			res = 1;
		}
		else
			debug_printf ("error: '%s' is not a valid %s IP\n", ip_string,
						  ip_class);
	}
	else
	{
		ina.s_addr = ip->addr;
		debug_printf ("%s IP is set to %s\n", ip_class, inet_ntoa (ina));
	}

	return res;
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
vNetworkSendBeaconToServer (void)
{
	if (env.e.reader_id && env.e.ip_server.addr && env.e.ip_server_port
		&& vNetworkSocketBuf && vNetworkSocket)
	{
		vNetworkSocketBuf->payload = &g_Beacon;
		udp_sendto (vNetworkSocket,
					vNetworkSocketBuf,
					&env.e.ip_server, (u16_t) env.e.ip_server_port);
	}
}

void
vNetworkDumpHex (const char *data, unsigned int length)
{
	unsigned char c;

	while (length--)
	{
		c = (unsigned char) (*data++);
		debug_printf ("%X%X%s", (c >> 4) & 0xF, c & 0xF, length ? ":" : "");
	}
}

void
vNetworkDumpConfig (void)
{
	debug_printf ("\nActive Network Configuration:\n");

	debug_printf ("\tIP          = %s\n", vNetworkNTOA (EMAC_if.ip_addr));
	debug_printf ("\tNetmask     = %s\n", vNetworkNTOA (EMAC_if.netmask));
	debug_printf ("\tGateway     = %s\n", vNetworkNTOA (EMAC_if.gw));

	debug_printf ("\nStored Network Configuration:\n"
				  "\t%s configuration [%i]\n",
				  vNetworkConfigName (env.e.ip_autoconfig),
				  env.e.ip_autoconfig);

	debug_printf ("\tMAC         = ");
	vNetworkDumpHex (cMACAddress, sizeof (cMACAddress));
	debug_printf ("\n");

	debug_printf ("\tIP          = %s\n", vNetworkNTOA (env.e.ip_host));
	debug_printf ("\tNetmask     = %s\n", vNetworkNTOA (env.e.ip_netmask));
	debug_printf ("\tGateway     = %s\n", vNetworkNTOA (env.e.ip_gateway));
	debug_printf ("\tServer      = %s:%u [UDP]\n",
				  vNetworkNTOA (env.e.ip_server), env.e.ip_server_port);
	debug_printf ("\n");
}

static void
vNetworkReceive (void *arg, struct udp_pcb *pcb, struct pbuf *p,
				 struct ip_addr *addr, u16_t port)
{
	int dbg;
	struct pbuf *buf;
	const char msg[] = "Hello World!\n\r.";

	if (!p)
		return;

	if ((dbg = PtGetDebugLevel ()) != 0)
	{
		debug_printf ("RX'ed %i bytes from %s:%i\n", p->len,
					  vNetworkNTOA (*addr), port);

		if (p->len == p->tot_len)
			hex_dump (p->payload, 0, p->len);
	}

	buf = pbuf_alloc (PBUF_TRANSPORT, sizeof (msg), PBUF_REF);
	if (buf)
	{
		buf->payload = &msg;
		udp_sendto (pcb, buf, addr, port);
		pbuf_free (buf);
	}

	pbuf_free (p);
}

static void
vNetworkThread (void *pvParameters)
{
	(void) pvParameters;
	struct ip_addr xIpAddr, xNetMask, xGateway;

	/* Initialize lwIP and its interface layer. */
	lwip_init ();

	/* Execute IP config settings */
	switch (env.e.ip_autoconfig)
	{
		case IP_AUTOCONFIG_STATIC_IP:
			xIpAddr = env.e.ip_host;
			xNetMask = env.e.ip_netmask;
			xGateway = env.e.ip_gateway;
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

	/* setup server response UDP packet */
	vNetworkSocket = udp_new ();
	udp_bind (vNetworkSocket, IP_ADDR_ANY, DEFAULT_SERVER_PORT);
	udp_recv (vNetworkSocket, vNetworkReceive, NULL);
	vNetworkSocketBuf =
		pbuf_alloc (PBUF_TRANSPORT, sizeof (g_Beacon), PBUF_REF);

	while (pdTRUE)
	{
		int cnt = 0;

		vTaskDelay (DHCP_FINE_TIMER_MSECS / portTICK_RATE_MS);

		if (env.e.ip_autoconfig == IP_AUTOCONFIG_DHCP)
		{
			dhcp_fine_tmr ();
			cnt += DHCP_FINE_TIMER_MSECS;
			if (cnt >= (DHCP_COARSE_TIMER_SECS * 1000))
			{
				dhcp_coarse_tmr ();
				cnt = 0;
			}
		}
	}
}

/*------------------------------------------------------------*/

void
vNetworkResetDefaultSettings (void)
{
	bzero (&env, sizeof (env));
	env.e.reader_id = 0;
	env.e.ip_autoconfig = IP_AUTOCONFIG_READER_ID;
	env.e.ip_server_port = DEFAULT_SERVER_PORT;
	env.e.filter_duplicates = pdTRUE;
	IP4_ADDR (&env.e.ip_host, 10, 254, 0, 0);
	IP4_ADDR (&env.e.ip_netmask, 255, 255, 0, 0);
	IP4_ADDR (&env.e.ip_gateway, 10, 254, 0, 1);
	IP4_ADDR (&env.e.ip_server, 10, 254, 0, 1);
}

/*------------------------------------------------------------*/

void
vNetworkInit (void)
{
	unsigned int mac_l;
	xTaskHandle networkTaskHandle = NULL;

	/* If no previous environment exists - create a new, but don't store it */
	env_init ();
	if (!env_load ())
		vNetworkResetDefaultSettings ();

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
