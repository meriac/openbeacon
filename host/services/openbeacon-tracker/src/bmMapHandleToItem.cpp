/***************************************************************
 *
 * OpenBeacon.org - OnAir protocol position estimator
 * accepts a text file with tag sightings per reader.
 *
 * See the following website for already decoded Sputnik data:
 * http://people.openpcd.org/meri/openbeacon/sputnik/data/24c3/
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

#include "bmMapHandleToItem.h"
#include "crc32.h"

bmMapHandleToItem::bmMapHandleToItem (void)
{
  m_ItemSize = 0;
  memset (&m_Map, 0, sizeof (m_Map));
}

bmMapHandleToItem::~bmMapHandleToItem (void)
{
  int i;
  bmHandleMap *map = m_Map;

  if (m_ItemSize > (int) sizeof (map->data))
    for (i = 0; i < HASH_MAP_INDEX_SIZE; i++)
      {
	if (map->handle)
	  free (map->data);
	map++;
      }
}

bool bmMapHandleToItem::SetItemSize (int ItemSize)
{
  if (ItemSize <= 0 || m_ItemSize)
    return false;
  else
    {
      m_ItemSize = ItemSize;
      return true;
    }
}

int
bmMapHandleToItem::GetItemSize (void)
{
  return m_ItemSize;
}

bmHandleMap *
bmMapHandleToItem::HashMap (bmHandle handle)
{
  int i;
  uint32_t crc32 = 0xffffffffL;

  for (i = 0; i < (int) sizeof (handle); i++)
    {
      crc32 = crc32_table[(uint8_t) crc32 ^ (uint8_t) handle] ^ (crc32 >> 8);
      handle >>= 8;
    }

  return &m_Map[(crc32 ^ 0xffffffffUL) % HASH_MAP_INDEX_SIZE];
}

void *
bmMapHandleToItem::Find (bmHandle handle, pthread_mutex_t ** mutex)
{
  bmHandleMap *map;
  int i;

  if (m_ItemSize && handle)
    {

      map = HashMap (handle);

      for (i = 0; i < HASH_MAP_INDEX_SIZE; i++)
	{
	  if (!map->handle)
	    return NULL;

	  if (map->handle == handle)
	    {
	      if (mutex)
		{
		  pthread_mutex_lock (&map->mutex);
		  *mutex = &map->mutex;
		}

	      if (i)
		printf ("found %lu after %i items\n",
			(long unsigned int) handle, i);

	      return (m_ItemSize <= (int) sizeof (map->data)) ? &map->data
		: map->data;
	    }

	  map++;
	  if (map >= &m_Map[HASH_MAP_INDEX_SIZE])
	    map = m_Map;
	};

      printf ("can't find %lu\n", (long unsigned int) handle);
    }
  return NULL;
}

void *
bmMapHandleToItem::Add (bmHandle handle, pthread_mutex_t ** mutex)
{
  bmHandleMap *map;
  int i;

  if (m_ItemSize && handle)
    {

      map = HashMap (handle);

      for (i = 0; i < HASH_MAP_INDEX_SIZE; i++)
	{
	  if (!map->handle)
	    {
	      map->mutex = PTHREAD_MUTEX_INITIALIZER;
	      if (mutex)
		{
		  pthread_mutex_lock (&map->mutex);
		  *mutex = &map->mutex;
		}
	      map->handle = handle;

	      if (m_ItemSize <= (int) sizeof (map->data))
		return &map->data;
	      else
		{
		  map->data = malloc (m_ItemSize);
		  if (m_ItemSize > (int) sizeof (map->data))
		    memset (map->data, 0, m_ItemSize);
		  return map->data;
		}
	    }
	  else if (map->handle == handle)
	    {
	      if (mutex)
		{
		  pthread_mutex_lock (&map->mutex);
		  *mutex = &map->mutex;
		}
	      return (m_ItemSize <= (int) sizeof (map->data)) ? &map->data
		: map->data;
	    }

	  map++;
	  if (map >= &m_Map[HASH_MAP_INDEX_SIZE])
	    map = m_Map;
	}
    }
  return NULL;
}

int
bmMapHandleToItem::IterateLocked (bmIterationCallback cb)
{
  int i, count;
  bmHandleMap *map;

  if (cb && m_ItemSize)
    {
      count = 0;
      map = m_Map;
      for (i = 0; i < HASH_MAP_INDEX_SIZE; i++)
	{
	  if (map->handle)
	    {
	      count++;
	      pthread_mutex_lock (&map->mutex);
	      cb ((m_ItemSize <= (int) sizeof (map->data)) ? &map->data
		  : map->data);
	      pthread_mutex_unlock (&map->mutex);
	    }
	  map++;
	}
    }
  else
    count = -1;

  return count;
}
