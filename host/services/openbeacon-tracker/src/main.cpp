/***************************************************************
 *
 * OpenBeacon.org - OnAir protocol position tracker
 *
 * uses a physical model and statistical analysis to calculate
 * positions of tags
 *
 * Copyright 2009-2011 Milosch Meriac <meriac@bitmanufaktur.de>
 *
 ***************************************************************/

/*
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published
 by the Free Software Foundation; version 3.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program; if not, write to the Free Software Foundation,
 Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <pcap.h>
#include <stdarg.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "bmMapHandleToItem.h"
#include "bmReaderPositions.h"
#include "openbeacon.h"

static bmMapHandleToItem g_map_reader, g_map_tag;
static int g_DoEstimation = 1;
static uint32_t g_reader_last_seen[READER_COUNT];
static bool g_first;

#define XXTEA_KEY_NONE 0
#define XXTEA_KEY_25C3_BETA 1
#define XXTEA_KEY_25C3_FINAL 2
#define XXTEA_KEY_24C3 3
#define XXTEA_KEY_23C3 4
#define XXTEA_KEY_CAMP07 5
#define XXTEA_KEY_LASTHOPE 6

/* proximity tag TEA encryption key */
static const long tea_keys[][4] = {
  {0x7013F569, 0x4417CA7E, 0x07AAA968, 0x822D7554},	/* 25C3 free beta version key */
  {0xbf0c3a08, 0x1d4228fc, 0x4244b2b0, 0x0b4492e9},	/* 25C3 final key  */
  {0xB4595344, 0xD3E119B6, 0xA814D0EC, 0xEFF5A24E},	/* 24C3 */
  {0xe107341e, 0xab99c57e, 0x48e17803, 0x52fb4d16},	/* 23C3 key */
  {0x8e7d6649, 0x7e82fa5b, 0xddd4541e, 0xe23742cb},	/* Camp 2007 key */
  {0x9c43725e, 0xad8ec2ab, 0x6ebad8db, 0xf29c3638}	/* The Last Hope - AMD Project (Attendee Metadata Project) */
};

#define TEA_KEY_COUNT (sizeof(tea_keys)/sizeof(tea_keys[0]))

static uint32_t g_tea_key_usage[TEA_KEY_COUNT+1];
static uint32_t g_valid_packets,g_total_crc_errors;

#define MX  ( (((z>>5)^(y<<2))+((y>>3)^(z<<4)))^((sum^y)+(tea_key[(p&3)^e]^z)) )
#define DELTA 0x9e3779b9UL

#define UDP_PORT 2342
#define STRENGTH_LEVELS_COUNT 4
#define TAGSIGHTINGFLAG_SHORT_SEQUENCE 0x01
#define TAGSIGHTINGFLAG_BUTTON_PRESS 0x02
#define TAGSIGHTING_BUTTON_TIME 5
#define TAG_MASS 3.0

#define MIN_AGGREGATION_SECONDS 5
#define MAX_AGGREGATION_SECONDS 16
#define RESET_TAG_POSITION_SECONDS (60*5)
#define READER_TIMEOUT_SECONDS (60*15)
#define AGGREGATION_TIMEOUT(strength) ((uint32_t)(MIN_AGGREGATION_SECONDS+(((MAX_AGGREGATION_SECONDS-MIN_AGGREGATION_SECONDS)/(STRENGTH_LEVELS_COUNT-1))*(strength))))

typedef struct
{
  double time;
  uint8_t strength[STRENGTH_LEVELS_COUNT];
} TAggregation;

typedef struct
{
  double px, py, Fx, Fy;
} TTagItemStrength;

typedef struct
{
  uint32_t id, sequence, button;
  double last_seen, last_calculated;
  double px, py, vx, vy;
  const TReaderItem *last_reader;
  TTagItemStrength power[STRENGTH_LEVELS_COUNT];
} TTagItem;

typedef struct
{
  uint32_t last_seen, fifo_pos;
  uint32_t tag_id, reader_id, reader_index;
  const TReaderItem *reader;
  TTagItem *tag;
  uint8_t strength[STRENGTH_LEVELS_COUNT];
  TAggregation levels[MAX_AGGREGATION_SECONDS];
} TEstimatorItem;

static void
diep (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  putc ('\n', stderr);
  exit (EXIT_FAILURE);
}

static inline u_int16_t
crc16 (const unsigned char *buffer, int size)
{
  u_int16_t crc = 0xFFFF;

  if (buffer && size)
    while (size--)
      {
	crc = (crc >> 8) | (crc << 8);
	crc ^= *buffer++;
	crc ^= ((unsigned char) crc) >> 4;
	crc ^= crc << 12;
	crc ^= (crc & 0xFF) << 5;
      }

  return crc;
}

static inline void
xxtea_decode (u_int32_t * v, u_int32_t length, const long *tea_key)
{
  u_int32_t z, y = v[0], sum = 0, e, p, q;

  if (!v || !tea_key)
    return;

  q = 6 + 52 / length;
  sum = q * DELTA;
  while (sum)
    {
      e = (sum >> 2) & 3;
      for (p = length - 1; p > 0; p--)
	z = v[p - 1], y = v[p] -= MX;

      z = v[length - 1];
      y = v[0] -= MX;
      sum -= DELTA;
    }
}

static inline void
shuffle_tx_byteorder (u_int32_t * v, u_int32_t len)
{
  while (len--)
    {
      *v = htonl (*v);
      v++;
    }
}

static inline double
microtime_calc (struct timeval *tv)
{
  return tv->tv_sec + (tv->tv_usec / 1000000.0);
}

static inline double
microtime (void)
{
  struct timeval tv;

  if (!gettimeofday (&tv, NULL))
    return microtime_calc (&tv);
  else
    return 0;
}

static inline void
ThreadIterateForceReset (void *Context, double timestamp, bool realtime)
{
  int i;
  TTagItemStrength *power = ((TTagItem *) Context)->power;

  for (i = 0; i < STRENGTH_LEVELS_COUNT; i++)
    {
      power->Fx = power->Fy = 0;
      power++;
    }
}

static inline void
ThreadIterateLocked (void *Context, double timestamp, bool realtime)
{
  int i, strength;
  double dist, dx, dy;
  TTagItem *tag;
  TTagItemStrength *power;
  TEstimatorItem *item = (TEstimatorItem *) Context;

  if (!item->reader
      || ((timestamp - item->last_seen) >= MAX_AGGREGATION_SECONDS))
    return;

  tag = item->tag;
  power = tag->power;

  for (i = 0; i < STRENGTH_LEVELS_COUNT; i++)
    {
      dx = item->reader->x - power->px;
      dy = item->reader->y - power->py;
      dist = sqrt (dx * dx + dy * dy);
      dx /= dist;
      dy /= dist;

      strength = item->strength[i];
      power->Fx += dx * strength;
      power->Fy += dy * strength;

      power++;
    }
}

static inline void
ThreadIterateForceCalculate (void *Context, double timestamp, bool realtime)
{
  int i;
  double delta_t, r, px, py, F;
  TTagItem *tag = (TTagItem *) Context;

  TTagItemStrength *power = tag->power;

  /* maintain delta time since last calculation */
  delta_t = timestamp - tag->last_calculated;
  tag->last_calculated = timestamp;

  /* ignore tags that were invisible for more than RESET_TAG_POSITION_SECONDS */
  if ((timestamp - tag->last_seen) > RESET_TAG_POSITION_SECONDS)
    {
      tag->vx = tag->vy = 0;
      return;
    }

  px = py = 0;

  if (tag->button)
    tag->button--;

  for (i = 0; i < STRENGTH_LEVELS_COUNT; i++)
    {
      r =
	sqrt (power->Fx * power->Fx +
	      power->Fy * power->Fy) / (STRENGTH_LEVELS_COUNT - i);
      if (r > 0)
	{
	  power->px += (power->Fx / r);
	  power->py += (power->Fy / r);
	}

      px += power->px;
      py += power->py;

      power++;
    }

  px /= STRENGTH_LEVELS_COUNT;
  py /= STRENGTH_LEVELS_COUNT;

  F = px - tag->px;
  tag->vx += (F * delta_t) / TAG_MASS;
  tag->px += F * delta_t * delta_t / (TAG_MASS * 2.0);

  F = py - tag->py;
  tag->vy += (F * delta_t) / TAG_MASS;
  tag->py += F * delta_t * delta_t / (TAG_MASS * 2.0);

  printf ("%s    {\"id\":%u,\"px\":%i,\"py\":%i}",
	  g_first ? "" : ",\n", tag->id, (int) tag->px, (int) tag->py);

  g_first = false;
}

static void
EstimationStep (double timestamp, bool realtime)
{
  int i, j;
  static uint32_t sequence = 0, packet_count_prev = 0, packet_rate = 0;
  static double timestamp_prev = 0;
  double delta_t, r;
  const TReaderItem *reader = g_ReaderList;

  if (realtime)
    usleep (200 * 1000);

  g_map_tag.IterateLocked (&ThreadIterateForceReset, timestamp, realtime);
  g_map_reader.IterateLocked (&ThreadIterateLocked, timestamp, realtime);

  /* tracking dump state in JSON format */
  printf ("{\n  \"id\":%u,\n"
          "  \"time\":%u,\n"
          "  \"packets\":{\n",
          sequence++,
          (uint32_t) timestamp);

  /* show per-key-statistics */
  if(g_valid_packets!=g_tea_key_usage[0])
  {
    printf ("    \"per_key\":[");
    for (i = 0; i < (int) TEA_KEY_COUNT; i++)
      printf ("%s%i",i?",":"",g_tea_key_usage[i]);
    printf ("],\n");
  }

  /* maintain packet rate statistics */
  r=microtime();
  delta_t=r-timestamp_prev;
  if(delta_t>=10)
  {
    timestamp_prev=r;
    packet_rate=(g_valid_packets-packet_count_prev)/delta_t;
    packet_count_prev=g_valid_packets;
  }

  /* print packet rate */
  printf ("    \"rate\":%i,\n",packet_rate);

  /* show CRC errors count */
  if(g_total_crc_errors)
    printf ("    \"crc_error\":%i,\n",g_total_crc_errors);

  /* show total packet count */
  printf ("    \"valid\":%i\n    },\n",g_valid_packets);

  /* show tag statistics */
  printf ("  \"tag\":[\n");

  /* show reader statistics */
  g_first = true;
  g_map_tag.IterateLocked (&ThreadIterateForceCalculate, timestamp, realtime);
  printf ("\n    ],\n  \"reader\":[\n");
  j = 0;
  for (i = 0; i < (int) READER_COUNT; i++)
    {
      if ((timestamp-g_reader_last_seen[i])<READER_TIMEOUT_SECONDS)
	printf
	  ("%s    {\"id\":%u,\"px\":%u,\"py\":%u,\"room\":%u,\"floor\":%u,\"group\":%u}",
	   j++ ? ",\n" : "", reader->id, (int) reader->x, (int) reader->y,
	   (int) reader->room, (int) reader->floor, (int) reader->group);
      reader++;
    }
  printf ("\n    ]\n},");

  /* propagate object on stdout */
  fflush (stdout);
}

static void *
ThreadEstimation (void *context)
{
  while (g_DoEstimation)
    EstimationStep (microtime (), true);
  return NULL;
}

static void
hex_dump (const void *data, unsigned int addr, unsigned int len)
{
  unsigned int start, i, j;
  const unsigned char *buf;
  char c;

  buf = (const unsigned char *) data;
  start = addr & ~0xf;

  for (j = 0; j < len; j += 16)
    {
      fprintf (stderr, "%08x:", start + j);

      for (i = 0; i < 16; i++)
	{
	  if (start + i + j >= addr && start + i + j < addr + len)
	    fprintf (stderr, " %02x", buf[start + i + j]);
	  else
	    fprintf (stderr, "   ");
	}
      fprintf (stderr, "  |");
      for (i = 0; i < 16; i++)
	{
	  if (start + i + j >= addr && start + i + j < addr + len)
	    {
	      c = buf[start + i + j];
	      if (c >= ' ' && c < 127)
		fprintf (stderr, "%c", c);
	      else
		fprintf (stderr, ".");
	    }
	  else
	    fprintf (stderr, " ");
	}
      fprintf (stderr, "|\n\r");
    }
}

static void
parse_packet (double timestamp, uint32_t reader_id, const void *data, int len,
	      bool decrypt)
{
  TTagItem *tag;
  TEstimatorItem *item;
  TAggregation *aggregation;
  uint32_t tag_id, tag_flags, tag_sequence;
  uint32_t delta_t, t, tag_sighting;
  pthread_mutex_t *item_mutex, *tag_mutex;
  int tag_strength, j, key_id;
  TBeaconEnvelope env;
  TTagItemStrength *power;
  double px,py;

  if (len != sizeof (env))
    return;

  key_id = -1;
  if (decrypt)
    {
      for (j = 0; j < (int) TEA_KEY_COUNT; j++)
	{
	  memcpy (&env, data, len);

	  /* decode packet */
	  shuffle_tx_byteorder (env.block, XXTEA_BLOCK_COUNT);
	  xxtea_decode (env.block, XXTEA_BLOCK_COUNT, tea_keys[j]);
	  shuffle_tx_byteorder (env.block, XXTEA_BLOCK_COUNT);

	  /* verify CRC */
	  if (crc16 (env.byte,
		     sizeof (env.pkt) - sizeof (env.pkt.crc)) ==
	      ntohs (env.pkt.crc))
	    {
	      key_id = j+1;
	      break;
	    }
	}
    }
  else
    {
      memcpy (&env, data, len);
      if (crc16 (env.byte,
		 sizeof (env.pkt) - sizeof (env.pkt.crc)) ==
	  ntohs (env.pkt.crc))
	key_id = 0;
    }


  if (key_id < 0)
    {
      g_total_crc_errors++;
      fprintf (stderr, "CRC[0x%04X] error from reader 0x%08X\n",
	(int) crc16 ((const unsigned char *) data, len), reader_id);
      hex_dump (data, 0, len);
      return;
    }
  else
    {
      /* maintain total packet count */
      g_valid_packets++;
      /* maintain statistics per key */
      g_tea_key_usage[key_id]++;
    }

  switch (env.pkt.proto)
    {
    case RFBPROTO_BEACONTRACKER_OLD:
      {
	if (env.old.proto2 != RFBPROTO_BEACONTRACKER_OLD2)
	  {
	    tag_strength = -1;
	    tag_sequence = 0;
	    fprintf (stderr, "unknown old packet protocol2[%i] key[%i] ",
		     env.old.proto2, key_id);
	  }
	else
	  {
	    tag_id = ntohl (env.old.oid);
	    tag_sequence = ntohl (env.old.seq);
	    tag_strength = env.old.strength / 0x55;
	    tag_flags = (env.old.flags & RFBFLAGS_SENSOR) ?
	      TAGSIGHTINGFLAG_BUTTON_PRESS : 0;
	  }
	break;
      }

    case RFBPROTO_BEACONTRACKER:
      {
	tag_id = ntohs (env.pkt.oid);
	tag_sequence = ntohl (env.pkt.p.tracker.seq);
	tag_strength = env.pkt.p.tracker.strength;
	tag_flags = (env.pkt.flags & RFBFLAGS_SENSOR) ?
	  TAGSIGHTINGFLAG_BUTTON_PRESS : 0;
      }
      break;

    case RFBPROTO_PROXTRACKER:
      tag_id = ntohs (env.pkt.oid);
      tag_sequence = ntohl (env.pkt.p.tracker.seq);
      tag_strength = env.pkt.p.tracker.strength & (STRENGTH_LEVELS_COUNT - 1);
      tag_flags = (env.pkt.flags & RFBFLAGS_SENSOR) ?
	TAGSIGHTINGFLAG_BUTTON_PRESS : 0;
      break;

    case RFBPROTO_PROXREPORT:
      tag_id = ntohs (env.pkt.oid);
      tag_flags = (env.pkt.flags & RFBFLAGS_SENSOR) ?
	TAGSIGHTINGFLAG_BUTTON_PRESS : 0;

      tag_sequence = ntohs (env.pkt.p.prox.seq);
      tag_strength = (STRENGTH_LEVELS_COUNT - 1);
      tag_flags |= TAGSIGHTINGFLAG_SHORT_SEQUENCE;

      for (j = 0; j < PROX_MAX; j++)
	{
	  tag_sighting = (ntohs (env.pkt.p.prox.oid_prox[j]));
	  if (tag_sighting)
	    {
	    }
	}
      break;

    case RFBPROTO_READER_ANNOUNCE:
      tag_strength = -1;
      tag_sequence = 0;
      break;

    default:
      fprintf (stderr, "unknown packet protocol[%03i] key[%i] ",
	       env.pkt.proto, key_id);
      hex_dump (&env, 0, sizeof (env));
      tag_strength = -1;
      tag_sequence = 0;
      tag_id = 0;
    }

  if ((tag_strength >= 0)
      && (item =
	  (TEstimatorItem *)
	  g_map_reader.Add ((((uint64_t) reader_id) << 32) |
			    tag_id, &item_mutex)) != NULL)
    {
      /* initialize on first occurence */
      if (!item->tag_id)
	{
	  item->tag_id = tag_id;

	  for (t = 0; t < READER_COUNT; t++)
	    if (g_ReaderList[t].id == reader_id)
	      break;
	  if (t < READER_COUNT)
	    {
	      item->reader = &g_ReaderList[t];
	      item->reader_id = reader_id;
	      item->reader_index = t;
	      item->last_seen = 0;
	    }
	  else
	    {
	      fprintf (stderr, "unknown reader 0x%08X\n", reader_id);
	      item->reader = NULL;
	      item->reader_id = 0;
	      item->reader_index = 0;
	      reader_id = 0;
	    }
	}

      /* increment reader stats for each packet */
      if (item->reader_id)
	g_reader_last_seen[item->reader_index] = timestamp;

      if ((tag = (TTagItem *) g_map_tag.Add (tag_id, &tag_mutex)) == NULL)
	diep ("can't add tag");
      else
	{
	  /* on first occurence */
	  if (!tag->id)
	    {
	      fprintf (stderr, "new tag %u seen\n", tag_id);
	      tag->id = tag_id;
	      tag->last_calculated = timestamp;
	    }

	  tag->last_reader = item->reader;
	  tag->last_seen = timestamp;
	  /* TODO: fix wrapping of 16 bit sequence numbers */
	  if (tag_flags & TAGSIGHTINGFLAG_SHORT_SEQUENCE)
	    tag->sequence = (tag->sequence & ~0xFFFF) | tag_sequence;

	  item->tag = tag;
	  pthread_mutex_unlock (tag_mutex);
	}

      /* get time difference since last run */
      delta_t = timestamp - item->last_seen;

      if (delta_t)
	{
	  item->last_seen = timestamp;

	  if (delta_t >= MAX_AGGREGATION_SECONDS)
	  {
	    memset (&item->levels, 0, sizeof (item->levels));
	    memset (&item->strength, 0, sizeof (item->strength));
	    item->fifo_pos = 0;

	    if(delta_t >= RESET_TAG_POSITION_SECONDS)
	    {
	      /* reset tag origin to first reader seen */
	      px = item->reader->x;
	      py = item->reader->y;
	      tag->px = px;
	      tag->py = py;
	      power = tag->power;

	      for(t=0;t<STRENGTH_LEVELS_COUNT;t++)
	      {
		power->px = px;
		power->py = py;
		power++;
	      }
	    }

	    delta_t = 0;
	  }
	  else
	  {
	    item->fifo_pos++;
	    if (item->fifo_pos >= MAX_AGGREGATION_SECONDS)
	      item->fifo_pos = 0;
	  }
	}

      /* get current aggregation position */
      aggregation = &item->levels[item->fifo_pos];

      /* reset values to zero */
      if (delta_t)
	{
	  memset (aggregation, 0, sizeof (*aggregation));
	  aggregation->time = timestamp;
	}
      aggregation->strength[tag_strength]++;

      if (delta_t)
	{
	  memset (&item->strength, 0, sizeof (item->strength));
	  aggregation = item->levels;
	  for (t = 0; t < MAX_AGGREGATION_SECONDS; t++)
	    {
	      for (j = 0; j < STRENGTH_LEVELS_COUNT; j++)
		if ((timestamp - aggregation->time) <=
		    AGGREGATION_TIMEOUT (j))
		  item->strength[j] += aggregation->strength[j];
	      aggregation++;
	    }
	}

      if (tag_flags & TAGSIGHTINGFLAG_BUTTON_PRESS)
	tag->button = TAGSIGHTING_BUTTON_TIME;

      pthread_mutex_unlock (item_mutex);
    }
}

static int
parse_pcap (const char *file, bool realtime)
{
  int len, items;
  FILE *f;
  pcap_t *h;
  char error[PCAP_ERRBUF_SIZE];
  pcap_pkthdr header;
  const ip *ip_hdr;
  const udphdr *udp_hdr;
  const uint8_t *packet;
  uint32_t reader_id, timestamp_old, timestamp;
  TBeaconEnvelopeLog log;

  timestamp_old = 0;

  if ((h = pcap_open_offline (file, error)) == NULL)
    {
      f = fopen (file, "r");
      if (!f)
	diep ("failed to open '%s' PCAP file (%s)\n", file, error);
      else
	{
	  while (fread (&log, sizeof (log), 1, f) == 1)
	    {
	      timestamp = ntohl (log.timestamp);
	      if (timestamp != timestamp_old)
		{
		  timestamp_old = timestamp;
		  EstimationStep (timestamp, realtime);
		}
	      parse_packet (timestamp, ntohl (log.ip), &log.env,
			    sizeof (log.env), false);
	    }
	  fclose (f);
	}
    }
  else
    {
      /* iterate over all IPv4 UDP packets */
      while ((packet = (const uint8_t *) pcap_next (h, &header)) != NULL)
	/* check for ethernet protocol */
	if ((((uint32_t) (packet[12]) << 8) | packet[13]) == 0x0800)
	  {
	    /* skip ethernet header */
	    packet += 14;
	    ip_hdr = (const ip *) packet;

	    /* if IPv4 UDP protocol */
	    if ((ip_hdr->ip_v == 0x4) && (ip_hdr->ip_p == 17))
	      {
		len = 4 * ip_hdr->ip_hl;
		packet += len;
		udp_hdr = (const udphdr *) packet;

		/* get UDP packet payload size */
		len = ntohs (udp_hdr->len) - sizeof (udphdr);
		packet += sizeof (udphdr);

		/* calculate amount of OpenBeacon packets */
		items = len / sizeof (TBeaconEnvelope);
		if (!items)
		  continue;

		/* run estimation every second */
		timestamp = microtime_calc (&header.ts);
		if ((timestamp - timestamp_old) >= 1)
		  {
		    timestamp_old = timestamp;
		    EstimationStep (timestamp, realtime);
		  }

		/* process all packets in this packet */
		reader_id = ntohl (ip_hdr->ip_src.s_addr);
		while (items--)
		  {
		    parse_packet (timestamp, reader_id,
				  packet, sizeof (TBeaconEnvelope), true);
		    packet += sizeof (TBeaconEnvelope);
		  }
	      }
	  }
    }
  return 0;
}

static int
listen_packets (void)
{
  int sock, items;
  uint32_t reader_id;
  double timestamp;
  TBeaconEnvelope beacons[100], *pkt;
  struct sockaddr_in si_me, si_other;
  socklen_t slen = sizeof (si_other);
  pthread_t thread_handle;

  if ((sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    diep (NULL, "socket");
  else
    {
      memset ((char *) &si_me, 0, sizeof (si_me));
      si_me.sin_family = AF_INET;
      si_me.sin_port = htons (UDP_PORT);
      si_me.sin_addr.s_addr = htonl (INADDR_ANY);

      if (bind (sock, (sockaddr *) & si_me, sizeof (si_me)) == -1)
	diep (NULL, "bind");

      pthread_create (&thread_handle, NULL, &ThreadEstimation, NULL);

      while (1)
	{
	  if ((items = recvfrom (sock, &beacons, sizeof (beacons), 0,
				 (sockaddr *) & si_other, &slen)) == -1)
	    diep (NULL, "recvfrom()");

	  items /= sizeof (TBeaconEnvelope);
	  if (!items)
	    continue;

	  pkt = beacons;
	  reader_id = ntohl (si_other.sin_addr.s_addr);

	  timestamp = microtime ();
	  while (items--)
	    parse_packet (timestamp, reader_id, pkt++, sizeof (*pkt), true);
	}
    }

  return 0;
}

int
main (int argc, char **argv)
{
  bool realtime;

  /* initialize statistics */
  g_valid_packets=g_total_crc_errors=0;
  memset(&g_tea_key_usage,0,sizeof(g_tea_key_usage));

  g_map_reader.SetItemSize (sizeof (TEstimatorItem));
  g_map_tag.SetItemSize (sizeof (TTagItem));

  /* check command line arguments */
  if (argc <= 1)
    return listen_packets ();
  else
    {
      realtime = (argc >= 3) ? (atoi (argv[2]) ? true : false) : false;
      return parse_pcap (argv[1], realtime);
    }
}
