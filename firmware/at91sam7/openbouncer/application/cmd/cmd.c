/***************************************************************
 *
 * OpenBeacon.org - OpenBeacon link layer protocol
 *
 * Copyright 2007 Angelo Cuccato <cuccato@web.de>
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
#include <beacontypes.h>
#include <led.h>

#include "cmd.h"
#include "commandline.h"
#include "cmd_main.h"
#include "login.h"
/**********************************************************************/

xTaskHandle xCmdRecvUsbTask;
/**********************************************************************/

// A task to read chars from USB
void
vCmdRecvUsbCode (void *pvParameters)
{
  char Temp;
  (void) pvParameters;

  for (;;)
    {
      if (vUSBRecvByte (&Temp, 1, 100))
	{
	  vLedSetRed (1);

	  if (UserLoginStatus == LoggedIn)
	    {
	      cmdlineDecodeInput (Temp);
	    }
	  if (UserLoginStatus != LoggedIn)
	    {
	      UserLogin (Temp);
	    }

	  vLedSetRed (0);
	}
    }
}

/**********************************************************************/

portBASE_TYPE
vCmdInit (void)
{
  vLedSetGreen (1);

  cmdlineInit ();
  cmd_Main_init ();

  UserLoginInit ();

  if (xTaskCreate
      (vCmdRecvUsbCode, (signed portCHAR *) "CMDUSB", TASK_CMD_STACK, NULL,
       TASK_CMD_PRIORITY, &xCmdRecvUsbTask) != pdPASS)
    {
      return 0;
    }

  return 1;
}
