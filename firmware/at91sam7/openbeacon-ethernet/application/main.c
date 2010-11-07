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
#include <ff.h>

#include "lwip/ip.h"
#include "lwip/ip_addr.h"
#include "led.h"
#include "env.h"
#include "cmd.h"
#include "proto.h"

/**********************************************************************/
#define SECTOR_BUFFER_SIZE 1024
/**********************************************************************/
static xQueueHandle xLogfile;
static uint8_t sector_buffer[SECTOR_BUFFER_SIZE];
static const char logfile[] = "LOGFILE.TXT";
static FATFS fatfs;

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
  UINT written;
  uint8_t data;
  static FIL fil;
  portTickType time, time_old;

  /* delay SD card init by 5 seconds */
  vTaskDelay (5000 / portTICK_RATE_MS);

  /* never fails - data init */
  memset (&fatfs, 0, sizeof (fatfs));
  f_mount (0, &fatfs);

  /* opening new file for write access */
  debug_printf ("\nCreating logfile (%s).\n", logfile);
  if (f_open (&fil, logfile, FA_WRITE | FA_CREATE_ALWAYS))
    debug_printf ("\nfailed to create file\n");
  else
    {
      /* Enable Debug output as we were able to open the log file */
      debug_printf ("OpenBeacon firmware version %s\nreader_id=%i.\n", VERSION, env.e.reader_id);
      PtSetDebugLevel (1);

      /* Storing clock ticks for flushing cache action */
      time_old = xTaskGetTickCount ();
      pos = 0;

      for (;;)
	if (xQueueReceive (xLogfile, &data, 100))
	  {
	    sector_buffer[pos++] = data;
	    if (pos == SECTOR_BUFFER_SIZE)
	      {
		pos = 0;
		if (f_write
		    (&fil, &sector_buffer, sizeof (sector_buffer), &written)
		    || written != sizeof (sector_buffer))
		  debug_printf ("\nfailed to write to logfile\n");
	      }

	    /* flush file every 5 seconds */
	    time = xTaskGetTickCount ();
	    if ((time - time_old) > (5000 / portTICK_RATE_MS))
	      {
		time_old = time;
		if (f_sync (&fil))
		  debug_printf ("\nfailed to flush to logfile\n");
	      }
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

  xLogfile = xQueueCreate (SECTOR_BUFFER_SIZE * 2, sizeof (char));

  xTaskCreate (vUSBCDCTask, (signed portCHAR *) "USB", TASK_USB_STACK,
	       NULL, TASK_USB_PRIORITY, NULL);

  xTaskCreate (vFileTask, (signed portCHAR *) "FILE", TASK_FILE_STACK,
	       NULL, TASK_FILE_PRIORITY, NULL);

  vTaskStartScheduler ();

  while (1);
}
