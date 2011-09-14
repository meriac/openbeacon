/***************************************************************
 *
 * OpenBeacon.org - persistent RAM support
 *
 * Copyright 2010 Milosch Meriac <meriac@openbeacon.de>
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
#include <openbeacon.h>
#ifdef ENABLE_PERSISTENT_INITIALIZATION
#include <persistent.h>

extern uint8_t __persistent_beg__;
extern uint8_t __persistent_end__;

static uint16_t g_crc16 __attribute__ ((section (".persistent_manage")));

void
persistent_update(void)
{
  uint32_t length;

  length = ((uint32_t) & __persistent_end__) -
    ((uint32_t) & __persistent_beg__);

  g_crc16 = icrc16 (&__persistent_beg__, length);
}

void
persistent_init (void)
{
  uint32_t length;

  length = ((uint32_t) & __persistent_end__) -
    ((uint32_t) & __persistent_beg__);

  /* initialize RAM if CRC is wrong */
  if (length && (g_crc16 != icrc16 (&__persistent_beg__, length)))
  {
    g_crc16 = 0;
    bzero (&__persistent_beg__, length);
  }
}

#endif /*ENABLE_PERSISTENT_INITIALIZATION */
