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
#include <stdlib.h>

#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <USB-CDC.h>
#include <task.h>
#include <beacontypes.h>
#include <board.h>
#include <dbgu.h>
#include <led.h>
#include <task.h>
#include <crc32.h>
#include <rnd.h>
#include <env.h>

#include "sdram.h"
#include "wifi.h"

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
    (1 << AT91C_ID_PIOA) | (1 << AT91C_ID_PIOB) | (1 << AT91C_ID_PIOC);
}

/**********************************************************************/
void
vApplicationIdleHook (void)
{
  /* Disable SDRAM clock and send SDRAM to self-refresh. The SDRAM will be
   * automatically reenabled on the next access.
   */
  AT91C_BASE_SDRC->SDRC_SRR = 1;
  /* Disable processor clock to set the core into Idle Mode. The clock will 
   * automatically be reenabled by any interrupt. */
  AT91C_BASE_PMC->PMC_SCDR = 1;

  /* Restart watchdog, has been enabled in Cstartup_SAM7.c */
  AT91F_WDTRestart(AT91C_BASE_WDTC);
}

/**********************************************************************/
#ifdef  SDRAM_TASK
static inline unsigned char
HexChar (unsigned char nibble)
{
  return nibble + ((nibble < 0x0A) ? '0' : ('A' - 0xA));
}

/**********************************************************************/
static inline void
sdram_test_task (void *parameter)
{
  volatile int i;
  (void) parameter;
  volatile unsigned int *pSdram = (unsigned int *) (SDRAM_BASE);
  unsigned int l, l1, data;

  printf ("SDRAM Init");
  vTaskDelay (10000 / portTICK_RATE_MS);
  printf ("Starting SDRAM test");

  for (i = 0; i < 4; i++)
    {
      SDRAM_BASE[i] = i;
    }

  if (pSdram[0] == 0x03020100)
    {
      printf ("Byte-wise SDRAM access OK\n");
    }
  else
    {
      printf ("Byte-wise SDRAM access not OK\n");
    }

  vTaskDelay (100 / portTICK_RATE_MS);

  i = 0;
  while (1)
    {
      printf ("Test Nr.%i:\n", ++i);

      printf ("\tWRITING 0xAA\n");
      memset ((void *) SDRAM_BASE, 0xAA, SDRAM_SIZE);
      printf ("\tREADING 0xAA\n");
      pSdram = (unsigned int *) (SDRAM_BASE);
      for (l = 0; l < (SDRAM_SIZE / 4); l++)
	{
	  data = (*pSdram++) ^ 0xAAAAAAAA;
	  if (data)
	    {
	      printf ("\t\terror at 0x%08X: 0x%08X\n", l * 4, data);
	      vTaskDelay (100 / portTICK_RATE_MS);
	    }
	}

      printf ("\tWRITING 0x55\n");
      memset ((void *) SDRAM_BASE, 0x55, SDRAM_SIZE);
      printf ("\tREADING 0x55\n");
      pSdram = (unsigned int *) (SDRAM_BASE);
      for (l = 0; l < (SDRAM_SIZE / 4); l++)
	{
	  data = (*pSdram++) ^ 0x55555555;
	  if (data)
	    {
	      printf ("\t\terror at 0x%08X: 0x%08X\n", l * 4, data);
	      vTaskDelay (100 / portTICK_RATE_MS);
	    }
	}

      printf ("\tWRITING Random Data\n");
      for (l = 0; l < (SDRAM_SIZE / 4); l++)
	(*pSdram++) = crc32 ((unsigned char *) &l, sizeof (l));
      printf ("\tREADING Random Data\n");
      for (l = 0; l < (SDRAM_SIZE / 4); l++)
	{
	  data = (*pSdram++) ^ crc32 ((unsigned char *) &l, sizeof (l));
	  if (data)
	    {
	      printf ("\t\terror at 0x%08X: 0x%08X\n", l * 4, data);
	      vTaskDelay (100 / portTICK_RATE_MS);
	    }
	}
      printf ("\tREADING Random Address\n");
      for (l = 0; l < (SDRAM_SIZE / 4); l++)
	{
	  l1 = crc32 ((unsigned char *) &l, sizeof (l)) % (SDRAM_SIZE / 4);
	  data = pSdram[l1] ^ crc32 ((unsigned char *) &l1, sizeof (l1));
	  if (data)
	    {
	      printf ("\t\terror at 0x%08X: 0x%08X\n", l1 * 4, data);
	      vTaskDelay (100 / portTICK_RATE_MS);
	    }
	}
    }
}
#endif /*SDRAM_TASK */

/**********************************************************************/
void __attribute__ ((noreturn)) mainloop (void)
{
  prvSetupHardware ();
  AT91F_RSTSetMode (AT91C_BASE_RSTC,
		    AT91F_RSTGetMode (AT91C_BASE_RSTC) | 0x11);

  /* If no previous environment exists - create a new, but don't store it */
  env_init ();
  if (!env_load ())
  {
    bzero (&env, sizeof (env));
    env.e.reader_id = 1;
  }

  vRndInit (env.e.reader_id);
  led_init ();
  wifi_init ();

  xTaskCreate (vUSBCDCTask, (signed portCHAR *) "USB", TASK_USB_STACK,
	       NULL, TASK_USB_PRIORITY, NULL);

#ifdef  SDRAM_TASK
  xTaskCreate (sdram_test_task, (signed portCHAR *) "SDRAM_DEMO 0", 512,
	       (void *) 0, NEAR_IDLE_PRIORITY, NULL);
#endif /*SDRAM_TASK */

  vTaskStartScheduler ();

  while (1);
}
