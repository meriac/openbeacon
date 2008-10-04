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

/* RF includes */
#include "proto.h"

/* Blinkenlights includes. */
#include "bprotocol.h"

static struct udp_pcb *b_pcb, *b_ret_pcb;
static struct ip_addr b_last_ipaddr;
static struct pbuf *b_ret_pbuf;
static BRFPacket rfpkg;

unsigned int b_rec_total = 0;
unsigned int b_rec_frames = 0;
unsigned int b_rec_setup = 0;
unsigned int sequence_seed = 0;
unsigned char last_lamp_val[MAX_LAMPS] = { 0 };

#define SUBSIZE ((sizeof(*sub) + sub->height * ((sub->width + 1) / 2)))

static int
b_parse_mcu_multiframe (mcu_multiframe_header_t * header, unsigned int maxlen)
{
  unsigned int i;
  mcu_subframe_header_t *sub = NULL;

  if (maxlen < sizeof (*header))
    return 0;

  if (sequence_seed == 0) {
    sequence_seed = (PtSwapLong (header->timestamp_l))
      - (unsigned int) (xTaskGetTickCount () / portTICK_RATE_MS);
 
    debug_printf ("seeding sequence: %lu\n", sequence_seed);
  }

//      debug_printf(" parsing mcu multiframe maxlen = %d\n", maxlen);

  maxlen -= sizeof(*header);

  while (maxlen)
    {
      if (!sub)
	sub = header->subframe;
      else
	sub = (mcu_subframe_header_t *) ((char *) sub + SUBSIZE);

      /* ntohs */
      sub->height = PtSwapShort (sub->height);
      sub->width = PtSwapShort (sub->width);

      if (sub->bpp != 4)
	{
	  maxlen -= SUBSIZE;
	  break;
	}

      b_rec_frames++;
      //debug_printf("subframe: bpp = 4, pkg rest size = %d, w %d, h %d!\n", maxlen, sub->width, sub->height);

      for (i = 0; i < env.e.n_lamps; i++)
	{
	  LampMap *m = env.e.lamp_map + i;
	  unsigned int index = (m->y * sub->width) + m->x;

	  if ((index / 2) >= maxlen - sizeof (*sub))
	    continue;

	  if (m->screen != sub->screen_id ||
	      m->x >= sub->width || m->y >= sub->height)
	    continue;

	  last_lamp_val[i] = sub->data[index / 2];
	  if (~index & 1)
	    last_lamp_val[i] >>= 4;

	  last_lamp_val[i] &= 0xf;
	}

      maxlen -= SUBSIZE;
    }

  return 0;
}
  
static void send_udp (char *buffer)
{
  udp_disconnect (b_ret_pcb);
  udp_connect (b_ret_pcb, &b_last_ipaddr, MCU_RESPONSE_PORT);
  b_ret_pbuf->payload = buffer;
  udp_send (b_ret_pcb, b_ret_pbuf);
}

static int
b_parse_mcu_setup (mcu_setup_header_t * header, int maxlen)
{
  return 0;
}

void
b_set_lamp_id (int lamp_id, int lamp_mac)
{
  memset (&rfpkg, 0, sizeof (rfpkg));
  rfpkg.cmd = RF_CMD_SET_LAMP_ID;

  rfpkg.mac = lamp_mac;
  rfpkg.wmcu_id = 0xff;
  rfpkg.set_lamp_id.id = lamp_id;
  rfpkg.set_lamp_id.wmcu_id = env.e.mcu_id;
  PtTransmit (&rfpkg , pdTRUE);
}

static inline void
b_set_gamma_curve (int lamp_mac, unsigned int block, unsigned short *gamma)
{
  int i;

  memset (&rfpkg, 0, sizeof (rfpkg));
  rfpkg.cmd = RF_CMD_SET_GAMMA;
  rfpkg.mac = lamp_mac;
  rfpkg.wmcu_id = env.e.mcu_id;
  rfpkg.set_gamma.block = block;

  for (i = 0; i < 8; i++)
    rfpkg.set_gamma.val[i] = gamma[i];

  debug_printf ("sending gamma table to %04x, block %d\n", lamp_mac, block);
  PtTransmit (&rfpkg, pdFALSE);
}

static inline void
b_write_gamma_curve (int lamp_mac)
{
  memset (&rfpkg, 0, sizeof (rfpkg));
  rfpkg.cmd = RF_CMD_WRITE_CONFIG;
  rfpkg.mac = lamp_mac;
  rfpkg.wmcu_id = env.e.mcu_id;
  PtTransmit (&rfpkg, pdFALSE);
}

static inline void
b_set_lamp_jitter (int lamp_mac, int jitter)
{
  memset (&rfpkg, 0, sizeof (rfpkg));
  rfpkg.cmd = RF_CMD_SET_JITTER;
  rfpkg.mac = lamp_mac;
  rfpkg.wmcu_id = env.e.mcu_id;
  rfpkg.set_jitter.jitter = jitter;
  PtTransmit (&rfpkg, pdFALSE);
}

static inline void
b_set_dimmer_delay (int lamp_mac, int delay)
{
  memset (&rfpkg, 0, sizeof (rfpkg));
  rfpkg.cmd = RF_CMD_SET_DIMMER_DELAY;
  rfpkg.mac = lamp_mac;
  rfpkg.wmcu_id = env.e.mcu_id;
  rfpkg.set_delay.delay = delay;
  PtTransmit (&rfpkg, pdFALSE);
}

static inline void
b_set_dimmer_control (int lamp_mac, int off)
{
  memset (&rfpkg, 0, sizeof (rfpkg));
  rfpkg.cmd = RF_CMD_SET_DIMMER_CONTROL;
  rfpkg.mac = lamp_mac;
  rfpkg.wmcu_id = env.e.mcu_id;
  rfpkg.dimmer_control.off = off;
  PtTransmit (&rfpkg, pdFALSE);
}

static inline void
b_set_assigned_lamps (mcu_devctrl_header_t * header, unsigned int len)
{
  int i;

  for (i = 0; i < MAX_LAMPS; i++)
    {
      LampMap *m;
      if (i * 4 * sizeof (int) >= len)
	break;

      m = env.e.lamp_map + i;
      m->mac 	= header->param[(i * 4) + 0];
      m->screen = header->param[(i * 4) + 1];
      m->x 	= header->param[(i * 4) + 2];
      m->y 	= header->param[(i * 4) + 3];

      b_set_lamp_id (i, m->mac);
      vTaskDelay (100 / portTICK_RATE_MS);
      debug_printf ("Lamp map %d -> MAC 0x%04x\n", i, m->mac);
    }

  env.e.n_lamps = i;
  debug_printf ("%d new assigned lamps set.\n", env.e.n_lamps);
  memset (last_lamp_val, 0, sizeof (last_lamp_val));
  vTaskDelay(100 / portTICK_RATE_MS);
  env_store();
}

static inline void
b_send_wdim_stats (unsigned int lamp_mac)
{
  memset (&rfpkg, 0, sizeof (rfpkg));
  rfpkg.cmd = RF_CMD_SEND_STATISTICS | RF_PKG_REPLY_WANTED;
  rfpkg.mac = lamp_mac;
  rfpkg.wmcu_id = env.e.mcu_id;
  PtTransmit (&rfpkg, pdFALSE);
}

static inline void
b_send_wmcu_stats (void)
{
  static char buffer[128] = { 0 };
  struct mcu_devctrl_header *hdr = (struct mcu_devctrl_header *) buffer;

  hdr->magic = PtSwapLong (MAGIC_MCU_RESPONSE);
  hdr->command = 0;
  hdr->mac = 0;

  hdr->param[0] = PtSwapLong (env.e.mcu_id);
  hdr->param[1] = PtSwapLong (b_rec_total);
  hdr->param[2] = PtSwapLong (rf_sent_broadcast);
  hdr->param[3] = PtSwapLong (rf_sent_unicast);
  hdr->param[4] = PtSwapLong (rf_rec);
  hdr->param[5] = PtSwapLong (PtGetRfJamDensity());
  hdr->param[6] = PtSwapLong (PtGetRfPowerLevel());
  hdr->param[7] = PtSwapLong (env.e.n_lamps);
  hdr->param[8] = PtSwapLong (VERSION_INT);
  hdr->param[9] = PtSwapLong ((xTaskGetTickCount() / portTICK_RATE_MS) / 1000);
  send_udp (buffer);
}

static inline void
b_send_wdim_reset (unsigned short lamp_mac)
{
  memset (&rfpkg, 0, sizeof (rfpkg));
  rfpkg.cmd = RF_CMD_RESET;
  rfpkg.mac = lamp_mac;
  rfpkg.wmcu_id = env.e.mcu_id;
  PtTransmit (&rfpkg, pdFALSE);
}

static int
b_parse_mcu_devctrl (mcu_devctrl_header_t * header, int maxlen)
{
  unsigned int i;

  b_rec_setup++;

  /* ntohs */
  header->command = PtSwapLong (header->command);
  header->mac = PtSwapLong (header->mac);
  header->value = PtSwapLong (header->value);

  if (header->mac != 0xffff && header->mac & 0x8000)
    {
      unsigned int id = header->mac & ~0x8000;
      if (id < env.e.n_lamps)
	header->mac = env.e.lamp_map[id].mac;
    }

  for (i = 0; i < (maxlen - sizeof (*header)) / 4; i++)
    header->param[i] = PtSwapLong (header->param[i]);

  switch (header->command)
    {
    case MCU_DEVCTRL_COMMAND_SET_MCU_ID:
      {
        env.e.mcu_id = header->value;
        debug_printf ("new MCU ID assigned: %d\n", env.e.mcu_id);
        vTaskDelay (100 / portTICK_RATE_MS);
	env_store ();
        break;
      }
    case MCU_DEVCTRL_COMMAND_SET_LAMP_ID:
      {
	int lamp_mac = header->mac;
	int lamp_id = header->value;
	b_set_lamp_id (lamp_id, lamp_mac);
	break;
      }
    case MCU_DEVCTRL_COMMAND_SET_GAMMA:
      {
	unsigned short i, gamma[8];
	int lamp_mac = header->mac;
	int block = header->value;

	for (i = 0; i < 8; i++)
	  gamma[i] = header->param[i];

	b_set_gamma_curve (lamp_mac, block, gamma);
	break;
      }
    case MCU_DEVCTRL_COMMAND_WRITE_CONFIG:
      {
	int lamp_mac = header->mac;
	b_write_gamma_curve (lamp_mac);
	break;
      }
    case MCU_DEVCTRL_COMMAND_SET_JITTER:
      {
	int lamp_mac = header->mac;
	int jitter = header->value;
	b_set_lamp_jitter (lamp_mac, jitter);
	break;
      }
    case MCU_DEVCTRL_COMMAND_SET_ASSIGNED_LAMPS:
      {
	b_set_assigned_lamps (header, maxlen - sizeof (*header));
	break;
      }
    case MCU_DEVCTRL_COMMAND_SEND_WDIM_STATS:
      {
	int lamp_mac = header->mac;
	b_send_wdim_stats (lamp_mac);
	break;
      }
    case MCU_DEVCTRL_COMMAND_SET_DIMMER_CONTROL:
      {
	debug_printf ("dimmer off-force 0x%04x %s\n", header->mac,
		      header->value ? "on" : "off");
	b_set_dimmer_control (header->mac, header->value);
	break;
      }
    case MCU_DEVCTRL_COMMAND_SET_DIMMER_DELAY:
      {
	debug_printf ("new dimmer delay for mac 0x%04x: %d ms\n", header->mac,
		      header->value);
	b_set_dimmer_delay (header->mac, header->value);
	break;
      }
    case MCU_DEVCTRL_COMMAND_SET_RF_POWER:
      {
        debug_printf ("new power level: %d\n", header->value);
	PtSetRfPowerLevel (header->value);
        break;
      }
    case MCU_DEVCTRL_COMMAND_SET_JAM_DENSITY:
      {
        debug_printf ("new jam density: %d\n", header->value);
	PtSetRfJamDensity (header->value);
        break;
      }
    case MCU_DEVCTRL_COMMAND_SEND_WMCU_STATS:
      {
	b_send_wmcu_stats ();
	break;
      }

    case MCU_DEVCTRL_COMMAND_RESET_MCU:
      {
        vTaskSuspendAll ();
	portENTER_CRITICAL ();
	/* endless loop to trigger watchdog reset */
	while (1) {};
        break;
      }
    case MCU_DEVCTRL_COMMAND_RESET_WDIM:
      {
        b_send_wdim_reset (header->mac);
        break;
      }
    case MCU_DEVCTRL_COMMAND_OUTPUT_RAW:
      {
	int i;

	rfpkg.cmd = header->param[0];
	rfpkg.mac = header->param[1];
	rfpkg.wmcu_id = header->param[2];
	for (i = 0; i < RF_PAYLOAD_SIZE; i++)
	  rfpkg.payload[i] = header->param[i + 3];

//        hex_dump((unsigned char *) &rfpkg, 0, sizeof(rfpkg));
	PtTransmit (&rfpkg, pdFALSE);
	break;
      }
    }

  return 0; //len;
}

static void
b_recv (void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr,
	u16_t port)
{
  unsigned char *payload = (unsigned char *) p->payload;
  unsigned int magic = 
      	payload[0] << 24 |
	payload[1] << 16 | 
	payload[2] << 8  |
	payload[3];

  b_rec_total++;

  if (p->len < sizeof (unsigned int))
    {
      pbuf_free (p);
      return;
    }

  switch (magic)
   {
     case MAGIC_MCU_MULTIFRAME:
       b_parse_mcu_multiframe ((mcu_multiframe_header_t *) (p->payload), p->len);
       break;
     case MAGIC_MCU_SETUP:
       b_parse_mcu_setup ((mcu_setup_header_t *) (p->payload), p->len);
	  break;
     case MAGIC_MCU_DEVCTRL:
       memcpy (&b_last_ipaddr, addr, sizeof (b_last_ipaddr));
       b_parse_mcu_devctrl ((mcu_devctrl_header_t *) (p->payload), p->len);
       break;
     default:
       debug_printf (" %s(): unknown magic %08x\n", __func__, magic);
    }

  pbuf_free (p);
}

void
b_parse_rfrx_pkg (BRFPacket * pkg)
{
  static char buffer[128] = { 0 };
  struct mcu_devctrl_header *hdr = (struct mcu_devctrl_header *) buffer;

  pkg->cmd &= ~0x40;
  hdr->magic = PtSwapLong (MAGIC_WDIM_RESPONSE);
  hdr->command = PtSwapLong (pkg->cmd);
  hdr->mac = PtSwapLong (pkg->mac);

  switch (pkg->cmd)
    {
    case RF_CMD_SEND_STATISTICS:
      debug_printf ("got dimmer stats from %04x: %d emi pulses, %d packets received\n",
		    pkg->mac, pkg->statistics.emi_pulses, pkg->statistics.packet_count);

      hdr->param[0] = PtSwapLong (pkg->statistics.packet_count);
      hdr->param[1] = PtSwapLong (pkg->statistics.emi_pulses);
      hdr->param[2] = PtSwapLong (pkg->statistics.pings_lost);
      hdr->param[3] = PtSwapLong (pkg->statistics.fw_version);
      hdr->param[4] = PtSwapLong (pkg->statistics.tick_count);
      send_udp (buffer);
      break;
    default:
      debug_printf ("unexpected dimmer return received, cmd = %d\n",
		    pkg->cmd);
      break;
    }
}

void
bprotocol_init (void)
{
  b_pcb = udp_new ();
  b_ret_pcb = udp_new ();
  b_ret_pbuf = pbuf_alloc (PBUF_TRANSPORT, 128, PBUF_REF);

  udp_recv (b_pcb, b_recv, NULL);
  udp_bind (b_pcb, IP_ADDR_ANY, MCU_LISTENER_PORT);
}
