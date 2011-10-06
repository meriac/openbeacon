/***************************************************************
 *
 * OpenBeacon.org - printf-to-buffer IO redirection
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
#include <customIO.h>

BOOL default_putchar (uint8_t data) LINKTO(cIO_putchchar);

static uint32_t cIOsize;
static uint8_t* cIObuffer;

BOOL
cIO_putchchar (uint8_t data)
{
  /* conditionally redirect printf IO to buffer */
  if(cIObuffer)
  {
    if (cIOsize)
    {
	cIOsize--;
	*cIObuffer++ = data;
	return TRUE;
    }
    else
	return FALSE;
  }
  else
    return UARTSendChar (data);
}

uint32_t
cIO_snprintf (char *str, uint32_t size, const char *fmt, ...)
{
  uint32_t res;
  va_list args;

  if(str && size)
  {
    cIOsize = size;
    cIObuffer = (uint8_t*)str;

    va_start (args, fmt);
    tiny_vsprintf (fmt, args);
    va_end (args);

    /* calculate string size without termination */
    res = size - cIOsize;

    /* add string termination if space left */
    if(cIOsize)
      *cIObuffer=0;
  }
  else
    res = 0;

  cIOsize = 0;
  cIObuffer = NULL;

  return res;
}
