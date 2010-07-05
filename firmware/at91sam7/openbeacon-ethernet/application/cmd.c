/***************************************************************
 *
 * OpenBeacon.org - OpenBeacon command line interface
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
#include <FreeRTOS.h>
#include <USB-CDC.h>
#include <semphr.h>
#include <task.h>
#include <string.h>
#include <board.h>
#include <crc32.h>
#include "env.h"
#include "cmd.h"
#include "debug_printf.h"


void
vCmdProcess (const char *cmdline)
{
  debug_printf ("CMD: %s\n\r", cmdline);
}

void
vCmdTask (void *pvParameters)
{
  (void) pvParameters;
  portCHAR cByte;
  portBASE_TYPE len = 0;
  static char next_command[MAX_CMD_LINE_LENGTH];

  for (;;)
    {
      if (vUSBRecvByte (&cByte, 1, 100))
	switch (cByte)
	  {
	  case '\n':
	  case '\r':
	    if (len)
	      {
		next_command[len] = 0;
		len = 0;
		vCmdProcess (next_command);
	      }
	    break;
	  default:
	    vUSBSendByte (cByte);
	    next_command[len++] = cByte;
	  }
    }
}

void
PtCmdInit (void)
{
  xTaskCreate (vCmdTask, (signed portCHAR *) "CMD", TASK_CMD_STACK,
	       NULL, TASK_CMD_PRIORITY, NULL);
}
