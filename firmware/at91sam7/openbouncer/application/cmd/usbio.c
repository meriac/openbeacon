/***************************************************************
 *
 * OpenBeacon.org - ...
 *
 * Copyright 2007 Milosch Meriac <meriac@openbeacon.de>
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

#include <AT91SAM7.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <USB-CDC.h>
#include <board.h>

//#include "proto.h"
//#include "env.h"
#include "usbio.h"
#include "dbgu.h"		// for: AT91F_DBGU_Frame
//#include "openbeacon.h"
//#include "led.h"

/**********************************************************************/

char rs232_buffer[1024];
unsigned int rs232_buffer_pos = 0;

/**********************************************************************/

void
vSendByte (char data)
{
  if (rs232_buffer_pos < sizeof (rs232_buffer))
    rs232_buffer[rs232_buffer_pos++] = data;

  if (data == '\r')
    {
      AT91F_DBGU_Frame (rs232_buffer, rs232_buffer_pos);
      rs232_buffer_pos = 0;
    }

  vUSBSendByte (data);
}

/**********************************************************************/

void
DumpUIntToUSB (unsigned int data)
{
  int i = 0;
  unsigned char buffer[10], *p = &buffer[sizeof (buffer)];

  do
    {
      *--p = '0' + (unsigned char) (data % 10);
      data /= 10;
      i++;
    }
  while (data);

  while (i--)
    vSendByte (*p++);
}

/**********************************************************************/

void
DumpStringToUSB (char *text)
{
  unsigned char data;

  if (text)
    while ((data = *text++) != 0)
      vSendByte (data);
}

/**********************************************************************/

static inline unsigned char
HexChar (unsigned char nibble)
{
  return nibble + ((nibble < 0x0A) ? '0' : ('A' - 0xA));
}

/**********************************************************************/

void
DumpBufferToUSB (char *buffer, int len)
{
  int i;

  for (i = 0; i < len; i++)
    {
      vSendByte (HexChar (*buffer >> 4));
      vSendByte (HexChar (*buffer++ & 0xf));
    }
}

/**********************************************************************/

int
atoiEx (const char *nptr)
{
  int sign = 1, i = 0, curval = 0;

  while (nptr[i] == ' ' && nptr[i] != 0)
    i++;

  if (nptr[i] != 0)
    {
      if (nptr[i] == '-')
	{
	  sign *= -1;
	  i++;
	}
      else if (nptr[i] == '+')
	i++;

      while (nptr[i] != 0 && nptr[i] >= '0' && nptr[i] <= '9')
	curval = curval * 10 + (nptr[i++] - '0');
    }

  return sign * curval;
}

/**********************************************************************/
