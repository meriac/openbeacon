/***************************************************************
 *
 * OpenBeacon.org - OnAir protocol position estimator
 *
 * uses a physical model and statistical analysis to calculate
 * positions of tags
 *
 * Copyright 2009 Milosch Meriac <meriac@bitmanufaktur.de>
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

#include <mysql.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "bmMapHandleToItem.h"
#include "bmReaderPositions.h"
#include "openbeacon.h"

static MYSQL *g_db = NULL;
static bmMapHandleToItem g_map_reader, g_map_tag;
static int g_DoEstimation = 1;

//
// proximity tag TEA encryption key
//
const long tea_key[4] = { 0xab94ec75, 0x160869c5, 0xfbf908da, 0x60bedc73 };

//const long tea_key[4] = { 0xB4595344, 0xD3E119B6, 0xA814D0EC, 0xEFF5A24E };
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
#define AGGREGATION_TIMEOUT(strength) ((uint32_t)(MIN_AGGREGATION_SECONDS+(((MAX_AGGREGATION_SECONDS-MIN_AGGREGATION_SECONDS)/(STRENGTH_LEVELS_COUNT-1))*(strength))))

typedef struct
{
  u_int32_t time;
  uint8_t strength[STRENGTH_LEVELS_COUNT];
} TAggregation;

typedef struct
{
  double px, py, Fx, Fy;
} TTagItemStrength;

typedef struct
{
  uint32_t id, sequence, button;
  uint32_t last_seen;
  double last_calculated;
  double px, py, vx, vy;
  const TReaderItem *last_reader;
  TTagItemStrength power[STRENGTH_LEVELS_COUNT];
} TTagItem;

typedef struct
{
  uint32_t last_seen, fifo_pos;
  uint32_t tag_id, reader_id;
  uint32_t timestamp;
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
  (void) putc ('\n', stderr);
  if (g_db)
    mysql_close (g_db);

  mysql_library_end ();
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

MYSQL *
db_connect (const char *dbname)
{
  MYSQL *db = mysql_init (NULL);
  if (!db)
    diep ("mysql_init failed: no memory");
  /*
   * Notice that the client and server use separate group names.
   * This is critical, because the server does not accept the
   * client's options, and vice versa.
   */
  mysql_options (db, MYSQL_READ_DEFAULT_GROUP, "opentracker");
  if (!mysql_real_connect (db, "127.0.0.1", NULL, NULL, dbname, 0, NULL, 0))
    diep ("mysql_real_connect failed: %s", mysql_error (db));

  return db;
}

static void
db_do_query (MYSQL * db, const char *query)
{
  if (mysql_query (db, query) != 0)
    goto err;

  if (mysql_field_count (db) > 0)
    {
      MYSQL_RES *res;
      MYSQL_ROW row, end_row;
      int num_fields;

      if (!(res = mysql_store_result (db)))
	goto err;
      num_fields = mysql_num_fields (res);
      while ((row = mysql_fetch_row (res)))
	{
	  (void) fputs (">> ", stdout);
	  for (end_row = row + num_fields; row < end_row; ++row)
	    (void) printf ("%s\t", row ? (char *) *row : "NULL");
	  (void) fputc ('\n', stdout);
	}
      (void) fputc ('\n', stdout);
      mysql_free_result (res);
    }

  return;

err:diep ("db_do_query failed: %s [%s]", mysql_error (db), query);
}

static inline double
microtime (void)
{
  struct timeval tv;

  if (!gettimeofday (&tv, NULL))
    return tv.tv_sec + (tv.tv_usec / 1000000.0);
  else
    return 0;
}

static inline void
ThreadIterateForceReset (void *Context)
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
ThreadIterateLocked (void *Context)
{
  int i, strength;
  double dist, dx, dy;
  TTagItem *tag;
  TTagItemStrength *power;
  TEstimatorItem *item = (TEstimatorItem *) Context;

  if (!item->reader
      || ((time (NULL) - item->last_seen) >= MAX_AGGREGATION_SECONDS))
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
ThreadIterateForceCalculate (void *Context)
{
  int i;
  char sql_req[1024];
  double t, delta_t, r, px, py, F;
  TTagItem *tag = (TTagItem *) Context;
  TTagItemStrength *power = tag->power;

  t = microtime ();
  delta_t = t - tag->last_calculated;
  tag->last_calculated = t;
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

  printf ("tag id=%u px=%f py=%f\n", tag->id, tag->px, tag->py);
  sprintf (sql_req,
	   "INSERT INTO distance ("
	   "tag_id,"
	   "x,y,"
	   "last_seen,"
	   "button"
	   ") VALUES(%u,%f,%f,NOW(),%u) ON DUPLICATE KEY UPDATE x=%f, y=%f, last_seen=NOW(), button=%u\n",
	   tag->id, tag->px, tag->py, tag->button, tag->px, tag->py,
	   tag->button);
  db_do_query (g_db, sql_req);
}

static void *
ThreadEstimation (void *context)
{
  while (g_DoEstimation)
    {
      g_map_tag.IterateLocked (&ThreadIterateForceReset);
      g_map_reader.IterateLocked (&ThreadIterateLocked);
      g_map_tag.IterateLocked (&ThreadIterateForceCalculate);
      sleep (1);
    }
  return NULL;
}

int
main (void)
{
  pthread_mutex_t *item_mutex, *tag_mutex;
  pthread_t thread_handle;
  struct sockaddr_in si_me, si_other;
  TTagItem *tag;
  TBeaconEnvelope beacons[100], *pkt;
  int j, items, sock, tag_strength;
  uint32_t reader_id, tag_id, tag_flags, tag_sequence, delta_t, t, timestamp,
    tag_sighting;
  socklen_t slen = sizeof (si_other);
  TEstimatorItem *item;
  TAggregation *aggregation;
  char sql_req[4096];

  mysql_library_init (0, NULL, NULL);
  g_db = db_connect (NULL);
  if (!g_db)
    diep (NULL, "can't connect to DB - halting program.");

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

      g_map_reader.SetItemSize (sizeof (TEstimatorItem));
      g_map_tag.SetItemSize (sizeof (TTagItem));

      while (1)
	{
	  if ((items = recvfrom (sock, &beacons, sizeof (beacons), 0,
				 (sockaddr *) & si_other, &slen)) == -1)
	    diep (NULL, "recvfrom()");

	  items /= sizeof (TBeaconEnvelope);
	  if (!items)
	    {
	      printf ("Too small packet received\n");
	      continue;
	    }

	  pkt = beacons;
	  reader_id = ntohl (si_other.sin_addr.s_addr);

	  while (items--)
	    {
	      shuffle_tx_byteorder (pkt->block, XXTEA_BLOCK_COUNT);
	      xxtea_decode (pkt->block, XXTEA_BLOCK_COUNT, tea_key);
	      shuffle_tx_byteorder (pkt->block, XXTEA_BLOCK_COUNT);

	      if (crc16
		  (pkt->byte,
		   sizeof (pkt->pkt) - sizeof (pkt->pkt.crc)) !=
		  ntohs (pkt->pkt.crc))
		{
		  printf ("CRC error from reader 0x%08X\n", reader_id);
		  continue;
		}

	      switch (pkt->pkt.proto)
		{
		case RFBPROTO_BEACONTRACKER:
		  {
		    tag_id = ntohs (pkt->pkt.oid);
		    tag_sequence = ntohl (pkt->pkt.p.tracker.seq);
		    tag_strength = pkt->pkt.p.tracker.strength;
		    tag_flags = (pkt->pkt.flags & RFBFLAGS_SENSOR) ?
		      TAGSIGHTINGFLAG_BUTTON_PRESS : 0;
		  }
		  break;

		case RFBPROTO_PROXTRACKER:
		  tag_id = ntohs (pkt->pkt.oid);
		  tag_sequence = ntohl (pkt->pkt.p.tracker.seq);
		  tag_strength = pkt->pkt.p.tracker.strength
		    & (STRENGTH_LEVELS_COUNT - 1);
		  tag_flags = (pkt->pkt.flags & RFBFLAGS_SENSOR) ?
		    TAGSIGHTINGFLAG_BUTTON_PRESS : 0;
		  break;

		case RFBPROTO_PROXREPORT:
		  tag_id = ntohs (pkt->pkt.oid);
		  tag_flags = (pkt->pkt.flags & RFBFLAGS_SENSOR) ?
		    TAGSIGHTINGFLAG_BUTTON_PRESS : 0;

		  tag_sequence = ntohs (pkt->pkt.p.prox.seq);
		  tag_strength = (STRENGTH_LEVELS_COUNT - 1);
		  tag_flags |= TAGSIGHTINGFLAG_SHORT_SEQUENCE;

		  for (j = 0; j < PROX_MAX; j++)
		    {
		      tag_sighting = (ntohs (pkt->pkt.p.prox.oid_prox[j]));
		      if (tag_sighting)
			{
			  sprintf (sql_req,
				   "INSERT INTO proximity ("
				   "tag_id,reader_id,tag_flags,"
				   "proximity_tag,proximity_count,proximity_strength"
				   ") VALUES(%u,%u,%u,%u,%u,%u)"
				   "ON DUPLICATE KEY UPDATE reader_id=%u, tag_flags=%u, proximity_tag=%u proximity_count=%u, proximity_strength=%u, time=NOW()\n",
				   tag_id, reader_id,
				   tag_flags,
				   (tag_sighting >> 0) & 0x7FF,
				   (tag_sighting >> 11) & 0x7,
				   (tag_sighting >> 14) & 0x3,
				   reader_id, tag_flags,
				   (tag_sighting >> 0) & 0x7FF,
				   (tag_sighting >> 11) & 0x7,
				   (tag_sighting >> 14) & 0x3);

//                            printf (sql_req);
//                            db_do_query (g_db, sql_req);
			}
		    }
		  break;
		case RFBPROTO_READER_ANNOUNCE:
		  tag_strength = -1;
		  tag_sequence = 0;
		  break;
		default:
		  printf ("unknown packet protocol [%i]\n", pkt->pkt.proto);
		  tag_strength = -1;
		  tag_sequence = 0;
		}

	      if ((tag_strength >= 0)
		  && (item =
		      (TEstimatorItem *)
		      g_map_reader.Add ((((uint64_t) reader_id) << 32) |
					tag_id, &item_mutex)) != NULL)
		{
		  timestamp = time (NULL);
		  item->timestamp = timestamp;

		  // initialize on first occurence
		  if (!item->tag_id)
		    {
		      item->tag_id = tag_id;

		      for (t = 0; t < READER_COUNT; t++)
			if (g_ReaderList[t].id == reader_id)
			  {
			    item->reader = &g_ReaderList[t];
			    break;
			  }
		      if (t < READER_COUNT)
			item->reader_id = reader_id;
		      else
			{
			  printf ("unknown reader 0x%08X\n", reader_id);
			  item->reader = NULL;
			  item->reader_id = 0;
			  reader_id = 0;
			}
		    }

		  if ((tag =
		       (TTagItem *) g_map_tag.Add (tag_id,
						   &tag_mutex)) == NULL)
		    diep ("can't add tag");
		  else
		    {
		      // on first occurence
		      if (!tag->id)
			{
			  printf ("new tag %u seen\n", tag_id);
			  tag->id = tag_id;
			  tag->last_calculated = microtime ();
			}
		      tag->last_reader = item->reader;
		      tag->last_seen = timestamp;
		      // TODO: fix wrapping of 16 bit sequence numbers
		      if (tag_flags & TAGSIGHTINGFLAG_SHORT_SEQUENCE)
			tag->sequence =
			  (tag->sequence & ~0xFFFF) | tag_sequence;

		      item->tag = tag;
		      pthread_mutex_unlock (tag_mutex);
		    }

		  delta_t = timestamp - item->last_seen;
		  if (delta_t >= MAX_AGGREGATION_SECONDS)
		    {
		      memset (&item->levels, 0, sizeof (item->levels));
		      memset (&item->strength, 0, sizeof (item->strength));
		      item->fifo_pos = 0;
		      item->last_seen = timestamp;
		    }
		  else
		    {
		      if (delta_t)
			{
			  item->fifo_pos++;
			  if (item->fifo_pos >= MAX_AGGREGATION_SECONDS)
			    item->fifo_pos = 0;
			}

		      aggregation = &item->levels[item->fifo_pos];

		      // reset values to zero
		      if (delta_t)
			{
			  memset (aggregation, 0, sizeof (*aggregation));
			  aggregation->time = timestamp;
			}
		      aggregation->strength[tag_strength]++;

		      if (delta_t)
			{
			  memset (&item->strength, 0,
				  sizeof (item->strength));
			  aggregation = item->levels;
			  for (t = 0; t < MAX_AGGREGATION_SECONDS; t++)
			    {
			      for (j = 0; j < STRENGTH_LEVELS_COUNT; j++)
				if ((timestamp - aggregation->time) <=
				    AGGREGATION_TIMEOUT (j))
				  item->strength[j] +=
				    aggregation->strength[j];
			      aggregation++;
			    }
			}
		    }

		  if (tag_flags & TAGSIGHTINGFLAG_BUTTON_PRESS)
		    tag->button = TAGSIGHTING_BUTTON_TIME;

		  printf
		    ("id:%04u reader:%03u proto:%03u strength:%u button:%03u levels:",
		     tag_id, reader_id & 0xFF, pkt->pkt.proto,
		     tag_strength, tag->button);
		  for (j = 0; j < STRENGTH_LEVELS_COUNT; j++)
		    printf ("%03u,", item->strength[j]);
		  printf ("\n");

		  fflush (stdout);

		  pthread_mutex_unlock (item_mutex);
		}

	      // process next packet
	      pkt++;
	    }
	}
    }
  return 0;
}
