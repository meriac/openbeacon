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

#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <USB-CDC.h>
#include <task.h>
#include <beacontypes.h>
#include <board.h>
#include <dbgu.h>
#include <led.h>
#include <task.h>

#include "proto.h"
#include "power.h"
#include "sdram.h"
#include "eink/eink.h"
#include "eink/eink_flash.h"
#include "touch/ad7147.h"
#include "accelerometer.h"
#include "sdcard.h"
#include "nfc/pn532.h"
#include "nfc/pn532_demo.h"
#include "nfc/picc_emu.h"
#include "ebook/ebook.h"


#define SECTOR_SIZE 512
#define SECTOR_COUNT 1
u_int8_t sector_buffer[SECTOR_SIZE * SECTOR_COUNT];


/**********************************************************************/
static inline void
prvSetupHardware (void)
{
  /*  When using the JTAG debugger the hardware is not always initialised to
     the correct default state.  This line just ensures that this does not
     cause all interrupts to be masked at the start. */
  AT91C_BASE_AIC->AIC_EOICR = 0;

  /*  Enable the peripheral clock. */
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOA;
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOB;
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOC;
}

/**********************************************************************/
void
vApplicationIdleHook (void)
{
}

void
watchdog_restart_task (void *parameter)
{
  (void) parameter;
  while (1)
    {
      /* Restart watchdog, has been enabled in Cstartup_SAM7.c */
      AT91F_WDTRestart (AT91C_BASE_WDTC);
      vTaskDelay (500 / portTICK_RATE_MS);
    }
}

void
demo_task (void *parameter)
{
  (void) parameter;

  led_set_red (1);
  vTaskDelay (500 / portTICK_RATE_MS);
  led_set_red (0);

  while (1)
    {
      vTaskDelay (1000 / portTICK_RATE_MS);
    }
}

/**********************************************************************/

static inline unsigned char
HexChar (unsigned char nibble)
{
  return nibble + ((nibble < 0x0A) ? '0' : ('A' - 0xA));
}

/**********************************************************************/

void
sdram_test_task (void *parameter)
{
  (void) parameter;
  volatile int i;

  vTaskDelay (1000 / portTICK_RATE_MS);
  volatile unsigned int *pSdram = (unsigned int *) SDRAM_BASE;
  for (i = 0; i < 4194304; i++)
    {
      pSdram[i] = 0xbeef0000 ^ i;
    }

  while (1)
    {
      int ok = 1;

      for (i = 0; i < 4194304; i++)
	{
	  u_int32_t test = pSdram[i];
	  if (test != (0xbeef0000 ^ i))
	    {
	      ok = 0;
	    }
	}
      led_set_red (0);
      led_set_green (0);
      vTaskDelay (100 / portTICK_RATE_MS);
      led_set_red (!ok);
      led_set_green (ok);
      if (ok)
	printf ("SDRAM ok\n");
    }
}

void
sdcard_test_task (void *parameter)
{
  int res, sector;

  (void) parameter;
  vTaskDelay (5000 / portTICK_RATE_MS);
  printf ("Here we go\n");

  if ((res = sdcard_open_card ()) != 0)
    {
      printf ("Can't open SDCARD [%i]\n", res);
      while (1)
	vTaskDelay (100 / portTICK_RATE_MS);
    }
  else
    {
      sector = 0;
      while (sdcard_disk_read (sector_buffer, 0, SECTOR_COUNT));
      {
	vTaskDelay (100);
	printf (".");
      }
      printf ("\n");

      while (1)
	{
	  memset (sector_buffer, 0, sizeof (sector_buffer));
	  if ((sector % 100) == 0)
	    printf ("%i kb\n", sector / 2);

	  res = sdcard_disk_read (sector_buffer, sector++, SECTOR_COUNT);

	  if (res)
	    printf ("\nresult=%03i DATA[0x%02X,0x%02X] reading SDCARD\n\n",
		    res, sector_buffer[0x1FE], sector_buffer[0x1FF]);
	}
    }
}

extern int eink_comm_test (u_int16_t reg1, u_int16_t reg2);
extern u_int16_t eink_read_register (u_int16_t reg);
#define READ_SIZE (256*1024)
void
flash_demo_task (void *parameter)
{
  (void) parameter;
  vTaskDelay (3000 / portTICK_RATE_MS);
  printf ("Here\n");

  int r = eink_flash_acquire ();
  if (r >= 0)
    {
      unsigned int ident = eink_flash_read_identification ();
      printf ("Device identification: %06X\n", ident);

      printf ("Starting read\n");
      eink_flash_read (0, (char *) SDRAM_BASE, READ_SIZE);
      eink_flash_release ();
      printf ("Dunnit\n");

    }
  else
    {
      switch ((enum eink_error) -r)
	{
	case EINK_ERROR_NOT_DETECTED:
	  printf ("eInk controller not detected\n");
	  break;
	case EINK_ERROR_NOT_SUPPORTED:
	  printf ("eInk controller not supported (unknown revision)\n");
	  break;
	case EINK_ERROR_COMMUNICATIONS_FAILURE:
	  printf
	    ("eInk controller communications failure, check FPC cable and connector\n");
	  break;
	case EINK_ERROR_CHECKSUM_ERROR:
	  printf ("eInk controller waveform flash checksum failure\n");
	  break;
	default:
	  printf ("eInk controller initialization: unknown error\n");
	  break;
	}
      if (eink_comm_test (0x148, 0x156))
	{
	  printf ("Comm test ok\n");
	}
      else
	{
	  printf ("Comm test not ok\n");
	  printf ("%04X\n", eink_read_register (0x148));
	}
    }

  while (1)
    {
      vTaskDelay (1000 / portTICK_RATE_MS);
    }
}


/**********************************************************************/
int
main (void)
{
  prvSetupHardware ();

  led_init ();

  if (!power_init ())
    {
      led_halt_blinking (1);
    }
  power_on ();

  xTaskCreate (watchdog_restart_task, (signed portCHAR *) "WATCHDOG",
	       TASK_WATCHDOG_STACK, NULL, TASK_WATCHDOG_PRIORITY, NULL);

  xTaskCreate (vUSBCDCTask, (signed portCHAR *) "USB", TASK_USB_STACK,
	       NULL, TASK_USB_PRIORITY, NULL);

  /* vInitProtocolLayer (); */

  //xTaskCreate (sdram_test_task, (signed portCHAR *) "SDRAM_DEMO", 512, NULL, NEAR_IDLE_PRIORITY, NULL);
  /*xTaskCreate (demo_task, (signed portCHAR *) "LED_DEMO", 512,
     NULL, NEAR_IDLE_PRIORITY, NULL); */
  xTaskCreate (sdcard_test_task, (signed portCHAR *) "SDCARD_DEMO", 1024,
	       NULL, NEAR_IDLE_PRIORITY, NULL);
  /*xTaskCreate (flash_demo_task, (signed portCHAR *) "FLASH DEMO", 512,
     NULL, NEAR_IDLE_PRIORITY, NULL); */
  /*xTaskCreate (pn532_demo_task, (signed portCHAR *) "PN532 DEMO TASK", TASK_PN532_STACK,
     NULL, TASK_PN532_PRIORITY, NULL); */


  sdram_init ();
  //eink_interface_init();
  //ad7147_init();
  //accelerometer_init();
  sdcard_init ();
  //pn532_init();
  //picc_emu_init();
  //ebook_init();

  led_set_green (1);

  vTaskStartScheduler ();

  return 0;
}
