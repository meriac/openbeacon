/*
 * libnfc_demo.c
 *
 *  Created on: 03.04.2009
 *      Author: henryk
 */

#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <beacontypes.h>

#include <task.h>
#include <USB-CDC.h>

#include <stdio.h>

#include "libnfc_demo.h"
#include "libnfc/libnfc.h"

static dev_info* pdi;

extern void DumpUIntToUSB(unsigned int i);
extern void hexdump(const unsigned char *data, size_t length);
static void libnfc_demo_task(void *parameter)
{
  (void) parameter;
  vTaskDelay(5000 / portTICK_RATE_MS);

  tag_info ti;

  // Try to open the NFC reader
  pdi = nfc_connect();

  if (pdi == INVALID_DEVICE_INFO)
  {
	  printf("Error connecting NFC reader\n");
	  goto out;
  }
  nfc_reader_init(pdi);

  // Drop the field for a while
  nfc_configure(pdi,DCO_ACTIVATE_FIELD,false);

  // Let the reader only try once to find a tag
  nfc_configure(pdi,DCO_INFINITE_LIST_PASSIVE,false);

  // Configure the CRC and Parity settings
  nfc_configure(pdi,DCO_HANDLE_CRC,true);
  nfc_configure(pdi,DCO_HANDLE_PARITY,true);

  printf("Go!\n");

  // Enable field so more power consuming cards can power themselves up
  nfc_configure(pdi,DCO_ACTIVATE_FIELD,true);

  /* FIXME: The following crashes (the old printf problem) */
  //printf("\nConnected to NFC reader: %s\n\n",pdi->acName);

  int a=0, b=0, current_time = xTaskGetTickCount(), last_time = xTaskGetTickCount();
  (void)a; (void)b; (void)current_time; (void)last_time;
  while(1) {
	  // Poll for a ISO14443A (MIFARE) tag
	  if (nfc_reader_list_passive(pdi,IM_ISO14443A_106,null,null,&ti))
	  {
#if 0
		  printf("The following (NFC) ISO14443A tag was found:\n\n");
		  printf("ATQA (SENS_RES):   "); fflush(stdout); hexdump(ti.tia.abtAtqa,2); printf("\n");
		  printf("UID (NFCID1):      "); fflush(stdout); hexdump(ti.tia.abtUid,ti.tia.uiUidLen); printf("\n");
		  printf("SAK (SEL_RES):     "); fflush(stdout); hexdump(&ti.tia.btSak,1); printf("\n");
		  if (ti.tia.uiAtsLen)
		  {
			  printf("ATS (ATR):         "); fflush(stdout);
			  hexdump(ti.tia.abtAts,ti.tia.uiAtsLen);
			  printf("\n");
		  }
	  }
#else
		  b++;
	  }
	  DumpUIntToUSB(a); printf(" Ok         \r"); fflush(stdout);
	  if( (current_time = xTaskGetTickCount()) > (last_time+1000) ) {
		  a = b; b = 0;
		  last_time = current_time;
	  }
#endif
  }

  // Poll for a Felica tag
  if (nfc_reader_list_passive(pdi,IM_FELICA_212,null,null,&ti) || nfc_reader_list_passive(pdi,IM_FELICA_424,null,null,&ti))
  {
	  // No test results yet
	  printf("felica\n");
  }

  // Poll for a ISO14443B tag
  if (nfc_reader_list_passive(pdi,IM_ISO14443B_106,null,null,&ti))
  {
	  // No test results yet
	  printf("iso14443b\n");
  }

  // Poll for a Jewel tag
  if (nfc_reader_list_passive(pdi,IM_JEWEL_106,null,null,&ti))
  {
	  // No test results yet
	  printf("jewel\n");
  }

  nfc_disconnect(pdi);

  printf("All done\n");

  out:
  while(1) {
	  vTaskDelay(1000 / portTICK_RATE_MS);
  }
}


void libnfc_demo_init(void)
{
	xTaskCreate (libnfc_demo_task, (signed portCHAR *) "LIBNFC DEMO TASK", 256, NULL, TASK_PN532_PRIORITY, NULL);
}
