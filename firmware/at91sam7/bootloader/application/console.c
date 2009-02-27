/***************************************************************
 *
 * OpenBeacon.org - USB boot loader console handling
 *
 * Copyright 2009 Milosch Meriac <meriac@bitmanufaktur.de>
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
/* Library includes. */
#include <FreeRTOS.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AT91SAM7.h>
#include <USB-CDC.h>
#include <board.h>
#include <task.h>
#include <led.h>

#include "console.h"

#define MAX_CMDLINE_LENGTH 80
#define MAX_ANSISEQUENCE_LENGTH 10

static char vConsoleCmd[MAX_CMDLINE_LENGTH + 1],
  vConsoleANSI[MAX_ANSISEQUENCE_LENGTH + 1];

int
puts (const char *message)
{
  char data;
  int res = 0;

  while ((data = *message++) != '\0')
    {
      vUSBSendByte (data);
      res++;
    }

  return res;
}

static void
puti (unsigned int data)
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

static void
msg (const char* msg)
{
    puts("\n\rbootloader: ");
    puts(msg);
}

static inline void
vConsoleProcessLine (void)
{
  int i;
  char c;

  const char *p,ani[]="|/-\\";

  switch (vConsoleCmd[0])
    {
    case 'A':
      for(i=0; i<20; i++)
      {
        p=ani;
        while((c=*p++)!=0)
        {
	  vUSBSendByte(c);
	  vTaskDelay(portTICK_RATE_MS * 100);
	  puts("\033[D");
        }
      }
      puts(" \033[D");
      break;
    case ':':
      puts ("iHEX");
      break;
    default:
      msg("command not found");
    }
}

static void
vConsoleTask (void *parameter)
{
  int ansipos, pos;
  char data;
  (void) parameter;

  ansipos = pos = 0;

  while (1)
    {
      if (vUSBRecvByte (&data, 1, 1000))
	{
	  if (data == 127)
	    {
	      ansipos = 0;
	      if (pos)
	      {
		pos--;
		puts("\033[D \033[D");
	      }
	    }
	  else if (ansipos)
	    {
	      if (data != ' ')
		{
		  vConsoleANSI[ansipos++] = data;
		  if ((data >= 64) && (data <= 126))
		    {
		    }
		}
	      if ((ansipos == MAX_ANSISEQUENCE_LENGTH) || (data < ' '))
		ansipos = 0;
	    }
	  else
	    {
	      if (data == 27)
		{
		  vConsoleANSI[0] = 27;
		  ansipos = 1;
		}
	      else if (data < ' ')
		switch (data)
		  {
		  case '\r':
		    if(pos)
		    {
			vConsoleCmd[pos] = 0;
			vConsoleProcessLine ();
			pos = 0;
		    }
		    puts ("\n\r#~> ");
		    break;
		  default:
		    puts (" =");
		    puti (data);
		    break;
		  }
	      else if (pos < MAX_CMDLINE_LENGTH)
		{
		  vUSBSendByte (data);
		  vConsoleCmd[pos++] = data;
		}
	    }
	}
    }
}

void
vInitConsole (void)
{
  xTaskCreate (vUSBCDCTask, (signed portCHAR *) "USB", TASK_USB_STACK,
	       NULL, TASK_USB_PRIORITY, NULL);
  xTaskCreate (vConsoleTask, (signed portCHAR *) "Console", TASK_CMD_STACK,
	       NULL, TASK_CMD_PRIORITY, NULL);
}
