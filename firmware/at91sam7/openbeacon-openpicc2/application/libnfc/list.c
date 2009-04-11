/*
 
Public platform independent Near Field Communication (NFC) library
Copyright (C) 2009, Roel Verdult
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libnfc.h"

static dev_info* pdi;

int main(int argc, const char* argv[])
{			
  tag_info ti;

  // Try to open the NFC reader
  pdi = nfc_connect();
  
  if (pdi == INVALID_DEVICE_INFO)
  {
    printf("Error connecting NFC reader\n");
    return 1;
  }
  nfc_reader_init(pdi);

  // Drop the field for a while
  nfc_configure(pdi,DCO_ACTIVATE_FIELD,false);

  // Let the reader only try once to find a tag
  nfc_configure(pdi,DCO_INFINITE_LIST_PASSIVE,false);

  // Configure the CRC and Parity settings
  nfc_configure(pdi,DCO_HANDLE_CRC,true);
  nfc_configure(pdi,DCO_HANDLE_PARITY,true);

  // Enable field so more power consuming cards can power themselves up
  nfc_configure(pdi,DCO_ACTIVATE_FIELD,true);

  printf("\nConnected to NFC reader: %s\n\n",pdi->acName);
  
  // Poll for a ISO14443A (MIFARE) tag
  if (nfc_reader_list_passive(pdi,IM_ISO14443A_106,null,null,&ti))
  {
    printf("The following (NFC) ISO14443A tag was found:\n\n");
    printf("%18s","ATQA (SENS_RES): "); print_hex(ti.tia.abtAtqa,2);
    printf("%18s","UID (NFCID1): "); print_hex(ti.tia.abtUid,ti.tia.uiUidLen);
    printf("%18s","SAK (SEL_RES): "); print_hex(&ti.tia.btSak,1);
    if (ti.tia.uiAtsLen)
    {
      printf("%18s","ATS (ATR): ");
      print_hex(ti.tia.abtAts,ti.tia.uiAtsLen);
    }
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
  return 1;
}
