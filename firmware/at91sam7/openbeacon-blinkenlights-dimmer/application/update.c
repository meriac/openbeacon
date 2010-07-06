/***************************************************************
 *
 * OpenBeacon.org - ATMEL Original Boot Loader Backup
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
/* Scheduler includes. */
#include <FreeRTOS.h>
#include <board.h>
#include <task.h>
#include <beacontypes.h>
#include <string.h>
#include "openbeacon.h"
#include "debug_print.h"
#include "env.h"
#include "led.h"
#include "USB-CDC.h"
#include "update.h"

extern void bootloader;
extern void bootloader_orig;
extern void bootloader_orig_end;

void DeviceRevertToUpdateMode (void)
{
  DumpStringToUSB ("resetting to default bootloader in update mode\n");
  vTaskDelay (500 / portTICK_RATE_MS);

  vTaskSuspendAll ();
  portENTER_CRITICAL ();

  vLedSetGreen (1);

  memcpy (&env.data, &bootloader, sizeof (env.data));
  memcpy (&env.data, &bootloader_orig,
          ((unsigned int) &bootloader_orig_end) -
          ((unsigned int) &bootloader_orig));

  env_flash_to (&bootloader);

  // endless loop to trigger watchdog reset    
  while(1){};
}

