/* bprotocol.c - FreeRTOS/lwip based implemenation of the Blinkenlights udp protocol
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
#include "SAM7_EMAC.h"
#include "env.h"

/* lwIP includes. */
#include "lwip/ip.h"
#include "lwip/udp.h"

/* Blinkenlights includes. */
#include "bprotocol.h"

/* RF includes */
#include "proto.h"

#define MAX_WIDTH 100
#define MAX_HEIGHT 100
#define MAX_CHANNELS 3
#define MAX_BPP 4

static struct udp_pcb *b_pcb;
static BRFPacket rfpkg;

unsigned char last_lamp_val[MAX_LAMPS] = { 0 };

#define SUBSIZE ((sizeof(*sub) + sub->height * ((sub->width + 1) / 2)))

static int b_parse_mcu_multiframe (mcu_multiframe_header_t *header, unsigned int maxlen)
{
	unsigned int i;
	mcu_subframe_header_t *sub = NULL;

	if (maxlen < sizeof(*header))
		return 0;

	maxlen -= sizeof(*header);

	debug_printf(" parsing mcu multiframe maxlen = %d\n", maxlen);

	while (maxlen) {
		if (!sub)
			sub = header->subframe;
		else
			sub = (mcu_subframe_header_t *) ((char *) sub + SUBSIZE);

		/* ntohs */
		sub->height = swapshort(sub->height);
		sub->width = swapshort(sub->width);

		if (sub->bpp != 4) {
			maxlen -= SUBSIZE;
			continue;
		}

		debug_printf("subframe: bpp = 4, pkg rest size = %d, w %d, h %d!\n", maxlen, sub->width, sub->height);

		for (i = 0; i < env.e.n_lamps; i++) {
			LampMap *m = env.e.lamp_map + i;
			unsigned int index = (m->y * sub->width) + m->x;

			if (index >= maxlen)
				continue;

			if (m->screen != sub->screen_id ||
			    m->x >= sub->width 		|| 
			    m->y >= sub->height)
				continue;

			last_lamp_val[i] = sub->data[index / 2];
			if (~index & 1)
				last_lamp_val[i] >>= 4;

			last_lamp_val[i] &= 0xf;
		}

		maxlen -= SUBSIZE;
	}

	memset(&rfpkg.payload, 0, RF_PAYLOAD_SIZE);

	for (i = 0; i < RF_PAYLOAD_SIZE; i++)
		rfpkg.payload[i] = (last_lamp_val[i * 2] & 0xf)
				 | (last_lamp_val[(i * 2) + 1] << 4);
	
	/* funk it. */
	memset(&rfpkg, 0, sizeof(rfpkg) - RF_PAYLOAD_SIZE);
	rfpkg.cmd = RF_CMD_SET_VALUES;
	rfpkg.wmcu_id = env.e.mcu_id;
	rfpkg.mac = 0xffff; /* send to all MACs */
	vnRFTransmitPacket(&rfpkg);
	return 0;
}

static int b_parse_mcu_setup (mcu_setup_header_t *header, int maxlen)
{
	int len = sizeof(*header);
	
	if (len > maxlen)
		return 0;

	if (header->width >= MAX_WIDTH ||
	    header->height >= MAX_HEIGHT ||
	    header->channels > MAX_CHANNELS)
	    return 0;

	len += header->width * header->height * header->channels;

	if (len > maxlen)
		return 0;

	return len;
}

static inline void b_set_lamp_id (int lamp_id, int lamp_mac)
{
	memset(&rfpkg, 0, sizeof(rfpkg));
	rfpkg.cmd = RF_CMD_SET_LAMP_ID;
	
	rfpkg.mac = lamp_mac;
	rfpkg.wmcu_id = 0xff;
	rfpkg.set_lamp_id.id = lamp_id;
	rfpkg.set_lamp_id.wmcu_id = env.e.mcu_id;
	vnRFTransmitPacket(&rfpkg);
}

static inline void b_set_gamma_curve (int lamp_mac, unsigned int block, unsigned short *gamma)
{
	int i;

	memset(&rfpkg, 0, sizeof(rfpkg));
	rfpkg.cmd = RF_CMD_SET_GAMMA;
	rfpkg.mac = lamp_mac;
	rfpkg.set_gamma.block = block;

	for (i = 0; i < 8; i++)
		rfpkg.set_gamma.val[i] = gamma[i];

	vnRFTransmitPacket(&rfpkg);
}

static inline void b_write_gamma_curve (int lamp_mac)
{
	memset(&rfpkg, 0, sizeof(rfpkg));
	rfpkg.cmd = RF_CMD_WRITE_CONFIG;
	rfpkg.mac = lamp_mac;
	vnRFTransmitPacket(&rfpkg);
}

static inline void b_set_lamp_jitter (int lamp_mac, int jitter)
{
	memset(&rfpkg, 0, sizeof(rfpkg));
	rfpkg.cmd = RF_CMD_SET_JITTER;
	rfpkg.mac = lamp_mac;
	rfpkg.set_jitter.jitter = jitter;
	vnRFTransmitPacket(&rfpkg);
}

static inline void b_set_assigned_lamps (unsigned int *map, unsigned int len)
{
	int i;

	for (i = 0; i < MAX_LAMPS; i++) {
		LampMap *m;
		if (i * 4 * sizeof(int) > len)
			break;

		m = env.e.lamp_map + i;
		m->mac    = map[(i * 4) + 0];
		m->screen = map[(i * 4) + 1];
		m->x      = map[(i * 4) + 2];
		m->y      = map[(i * 4) + 3];
		b_set_lamp_id (i, m->mac);
	}

	env.e.n_lamps = i - 1;
	env_store();
	debug_printf("%d new assigned lamps set.\n", env.e.n_lamps);
	memset(last_lamp_val, 0, sizeof(last_lamp_val));
}

static inline void b_send_wdim_stats(unsigned int lamp_mac)
{
	memset(&rfpkg, 0, sizeof(rfpkg));
	rfpkg.cmd = RF_CMD_SEND_STATISTICS;
	rfpkg.mac = lamp_mac;
	vnRFTransmitPacket(&rfpkg);
}

static int b_parse_mcu_devctrl(mcu_devctrl_header_t *header, int maxlen)
{
	unsigned int i;
//	int len = sizeof(*header);

//	if (len > maxlen)
//		return 0;

//	debug_printf(" %s() cmd %04x\n", __func__, header->command);

	/* ntohs */
	header->command	= swaplong(header->command);
	header->mac	= swaplong(header->command);
	header->value	= swaplong(header->value);

	for (i = 0; i < (maxlen - sizeof(*header)) / 4; i++)
		header->param[i] = swaplong(header->param[i]);

	switch (header->command) {
		case MCU_DEVCTRL_COMMAND_SET_MCU_ID:
			env.e.mcu_id = header->value;
			debug_printf("new MCU ID assigned: %d\n", env.e.mcu_id);
			env_store();
			break;
		case MCU_DEVCTRL_COMMAND_SET_LAMP_ID: {
			int lamp_mac = header->mac;
			int lamp_id = header->value;
			b_set_lamp_id(lamp_id, lamp_mac);
			break;
		}
		case MCU_DEVCTRL_COMMAND_SET_GAMMA: {
			unsigned short i, gamma[8];
			int lamp_mac = header->mac;
			int block = header->value;
			
			for (i = 0; i < 8; i++)
				gamma[i] = header->param[i];
			
			b_set_gamma_curve(lamp_mac, block, gamma);
			break;
		}
		case MCU_DEVCTRL_COMMAND_WRITE_CONFIG: {
			int lamp_mac = header->mac;
			b_write_gamma_curve(lamp_mac);
			break;
		}
		case MCU_DEVCTRL_COMMAND_SET_JITTER: {
			int lamp_mac = header->mac;
			int jitter = header->value;
			b_set_lamp_jitter(lamp_mac, jitter);
			break;
		}
		case MCU_DEVCTRL_COMMAND_SET_ASSIGNED_LAMPS: {
			b_set_assigned_lamps(header->param, maxlen - sizeof(*header));
			break;
		}
		case MCU_DEVCTRL_COMMAND_SEND_WDIM_STATS: {
			int lamp_mac = header->mac;
			b_send_wdim_stats(lamp_mac);
			break;
		}
		case MCU_DEVCTRL_COMMAND_OUTPUT_RAW: {
			int i;

			rfpkg.cmd = header->param[0];
			rfpkg.mac = header->param[1];
			rfpkg.wmcu_id = header->param[2];
			for (i = 0; i < RF_PAYLOAD_SIZE; i++)
				rfpkg.payload[i] = header->param[i+3];
		
//			hex_dump((unsigned char *) &rfpkg, 0, sizeof(rfpkg));
			vnRFTransmitPacket(&rfpkg);
			break;
		}
	}

	return 0; //len;
}

static void b_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
	unsigned int off = 0;
	unsigned char *payload = (unsigned char *) p->payload;

	if (p->len < sizeof(unsigned int)) { // || p->len > sizeof(payload)) {
		pbuf_free(p);
		return;
	}

	do {
		unsigned int consumed = 0;
		unsigned int magic = payload[off + 0] << 24 |
				     payload[off + 1] << 16  |
				     payload[off + 2] << 8 |
				     payload[off + 3];

		switch (magic) {
			case MAGIC_MCU_MULTIFRAME:
				consumed = b_parse_mcu_multiframe((mcu_multiframe_header_t *) (p->payload + off), p->len - off);
				break;
			case MAGIC_MCU_SETUP:
				consumed = b_parse_mcu_setup((mcu_setup_header_t *) (p->payload + off), p->len - off);
				break;
			case MAGIC_MCU_DEVCTRL:
				consumed = b_parse_mcu_devctrl((mcu_devctrl_header_t *) (p->payload + off), p->len - off);			
				break;
			default:
				debug_printf(" magic %04x\n", magic);			
		}

		if (consumed == 0)
			break;

		off += consumed;
	} while (off < p->len);

	pbuf_free(p);
}

void bprotocol_init(void)
{
	b_pcb = udp_new();

	udp_recv(b_pcb, b_recv, NULL);
	udp_bind(b_pcb, IP_ADDR_ANY, MCU_LISTENER_PORT);
	debug_printf("%s()\n", __func__);
}

