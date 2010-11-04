/***************************************************************
 *
 * OpenBeacon.org - main entry for 2.4GHz RFID USB reader
 *
 * Copyright 2007 Milosch Meriac <meriac@openbeacon.de>
 *
 * basically starts the USB task, initializes all IO ports
 * and introduces idle application hook to handle the HF traffic
 * from the nRF24L01 chip
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
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include <FreeRTOS.h>
#include <board.h>
#include <USB-CDC.h>
#include <task.h>
#include <dbgu.h>
#include <beacontypes.h>
#include <proto.h>
#include <network.h>
#include <queue.h>
#include <dosfs.h>

#include "lwip/ip.h"
#include "lwip/ip_addr.h"
#include "led.h"
#include "env.h"
#include "cmd.h"
#include "proto.h"
#include "sdcard.h"
#include "fat_helper.h"

/**********************************************************************/
static xQueueHandle xLogfile;
static uint8_t sector[SECTOR_SIZE];

/**********************************************************************/
static inline void
prvSetupHardware (void)
{
  /*  When using the JTAG debugger the hardware is not always initialised to
     the correct default state.  This line just ensures that this does not
     cause all interrupts to be masked at the start. */
  AT91C_BASE_AIC->AIC_EOICR = 0;

  /*  Enable the peripheral clock. */
  AT91C_BASE_PMC->PMC_PCER =
    (1 << AT91C_ID_PIOA) |
    (1 << AT91C_ID_PIOB) | (1 << AT91C_ID_EMAC) |
    (1 << AT91C_ID_SPI0) | (1 << AT91C_ID_SPI1);
}

/**********************************************************************/
void
vApplicationIdleHook (void)
{
#ifndef DISABLE_WATCHDOG
  /* Restart watchdog, has been enabled in Cstartup_SAM7.c */
  AT91F_WDTRestart (AT91C_BASE_WDTC);
#endif /*DISABLE_WATCHDOG */
}

/**********************************************************************/
void
vDebugSendHook (char data)
{
  vUSBSendByte (data);
  if (xLogfile)
    xQueueSend (xLogfile, &data, 0);
}

/**********************************************************************/
static void
vFileTask (void *parameter)
{
  uint32_t pos;
  uint8_t data;
  static FILEINFO fi;
  static const char logfile[] = "logfile.txt";

  vTaskDelay (5000 / portTICK_RATE_MS);
  do
    {
      debug_printf ("trying to open SDCARD...\n");
      vTaskDelay (2000 / portTICK_RATE_MS);
    }
  while (fat_init ());
  debug_printf ("\n...[SDCARD init done]\n\n");
  vTaskDelay (500 / portTICK_RATE_MS);

  if (fat_file_open (logfile, &fi))
    for (;;)
      {
	vTaskDelay (5000 / portTICK_RATE_MS);
	debug_printf ("\nFailed to open '%s' for writing!\n\n", logfile);
      }

  /* Enable Debug output as we were able to open the log file */
  debug_printf ("Starting logging for reader ID %i:\n", env.e.reader_id);
  PtSetDebugLevel(1);

  pos = 0;
  for (;;)
    if (xQueueReceive (xLogfile, &data, 100))
      {
	sector[pos++] = data;
	if (pos == SECTOR_SIZE)
	  {
	    pos = 0;
	    if (fat_file_append (&fi, &sector, sizeof (sector)) !=
		sizeof (sector))
	      debug_printf ("failed to flush to logfile");
	  }
      }
}

/**********************************************************************/
void __attribute__ ((noreturn)) mainloop (void)
{
  prvSetupHardware ();
  vLedInit ();

  PtCmdInit ();
  vNetworkInit ();
  PtInitProtocol ();

  xLogfile = xQueueCreate (SECTOR_SIZE * 4, sizeof (char));

  xTaskCreate (vUSBCDCTask, (signed portCHAR *) "USB", TASK_USB_STACK,
	       NULL, TASK_USB_PRIORITY, NULL);

  xTaskCreate (vFileTask, (signed portCHAR *) "FILE", TASK_FILE_STACK,
	       NULL, TASK_FILE_PRIORITY, NULL);

  vTaskStartScheduler ();

  while (1);
}
