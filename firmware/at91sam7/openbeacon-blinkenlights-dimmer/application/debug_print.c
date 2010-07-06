/***************************************************************
 *
 * OpenBeacon.org - OpenBeacon dimmer link layer protocol
 *
 * Copyright 2008 Milosch Meriac <meriac@openbeacon.de>
 *                Daniel Mack <daniel@caiaq.de>
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
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <string.h>
#include <math.h>
#include <board.h>
#include <beacontypes.h>
#include <USB-CDC.h>
#include "debug_print.h"

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
    vUSBSendByte (*p++);
}

static void printnibble(unsigned char nib)
{
  char a;

  nib &= 0xf;

  if (nib >= 10)
    a = nib - 10 + 'a';
  else
    a = nib + '0';

  vUSBSendByte(a);
}

static void printbyte(unsigned char byte)
{
  printnibble(byte >> 4);
  printnibble(byte);
}

void
DumpHexToUSB (unsigned int v, char bytes)
{
  while (bytes)
    {
      char c = v >> ((bytes - 1) * 8);
      printbyte(c);
      bytes--;
    }
}

void
DumpStringToUSB (const char *text)
{
  unsigned char data;

  if (text)
    while ((data = *text++) != 0) {
      vUSBSendByte (data);
      if (data == '\n')
        vUSBSendByte ('\r');
    }
}

