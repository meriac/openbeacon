/***************************************************************
 *
 * OpenBeacon.org - OpenBeacon link layer protocol
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
#include <semphr.h>
#include <task.h>
#include <string.h>
#include <board.h>
#include <beacontypes.h>
#include <sort.h>
#include "led.h"
#include "xxtea.h"
#include "proto.h"
#include "env.h"
#include "debug_printf.h"
#include "nRF24L01/nRF_HW.h"
#include "nRF24L01/nRF_CMD.h"
#include "nRF24L01/nRF_API.h"


#define TAG_ID_MASK (0x0000FFFFL)
#define ANNOUNCEMENT_TX_POWER 0

// set broadcast mac
const unsigned char broadcast_mac[NRF_MAX_MAC_SIZE] = { 0xDE, 0xAD, 0xBE, 0xEF, 42 };

xSemaphoreHandle PtSemaphore;
TBeaconReaderCommand reader_command;

static inline s_int8_t
PtInitNRF (void)
{
  if (!nRFAPI_Init (81, broadcast_mac, sizeof (broadcast_mac), 0 ))
    return 0;

  nRFAPI_SetPipeSizeRX (0, NRF_MAX_MAC_SIZE);
  nRFAPI_SetTxPower (ANNOUNCEMENT_TX_POWER);
  nRFAPI_PipesAck (ERX_P0);
  nRFAPI_SetRxMode (1);

  nRFCMD_CE (1);

  return 1;
}

static inline unsigned short
swapshort (unsigned short src)
{
  return (src >> 8) | (src << 8);
}

static inline unsigned long
swaplong (unsigned long src)
{
  return (src >> 24) | (src << 24) | ((src >> 8) & 0x0000FF00) | ((src << 8) & 0x00FF0000);
}

static inline void
DumpNrfRegisters (void)
{
  unsigned int size;
  unsigned char reg, buf[32], *p;

  debug_printf ("\n\rnRF24L01+ register dump:\n\r");
  reg = 0;
  while (reg <= 0x1C)
    {
      size = nRFCMD_GetRegSize (reg);
      if(size)
      {
	if (size > sizeof (buf))
	    size = sizeof (buf);

	nRFCMD_RegReadBuf (reg, buf, size);
	p = buf;

	// dump single register
	debug_printf("  R 0x%02X: ",reg);
	while (size--)
	    debug_printf(" 0x%02X",*p++);
	debug_printf("\n\r");
	vTaskDelay (200 / portTICK_RATE_MS);
      }
      reg++;
    }
    debug_printf("\n\r  ");
}

void
vnRFtaskRxTx (void *parameter)
{
  u_int8_t data[NRF_MAX_MAC_SIZE];
  (void) parameter;

  if (!PtInitNRF ())
    while(1)
    {
    vLedSetRed (1);
    vLedSetGreen (0);
    vTaskDelay (1000 / portTICK_RATE_MS);

    vLedSetRed (0);
    vLedSetGreen (1);
    vTaskDelay (1000 / portTICK_RATE_MS);
    }

  vLedSetGreen (1);
  vTaskDelay (5000 / portTICK_RATE_MS);

  DumpNrfRegisters();

  for (;;)
    {
      if (nRFCMD_WaitRx (10))
	{
	  vLedSetRed (1);

	  do
	    {
	      // read packet from nRF chip
	      nRFCMD_RegReadBuf (RD_RX_PLOAD, (unsigned char*)&data, sizeof (data));
	      debug_printf("RX: %2X %2X %2X %2X %2X\n",data[0],data[1],data[2],data[3],data[4]);
	    }
	  while ((nRFAPI_GetFifoStatus () & FIFO_RX_EMPTY) == 0);

	  vLedSetRed (0);
	}
      nRFAPI_ClearIRQ (MASK_IRQ_FLAGS);
    }
}

void
vInitProtocolLayer (void)
{
  PtSemaphore = NULL;
  vSemaphoreCreateBinary (PtSemaphore);

  xTaskCreate (vnRFtaskRxTx, (signed portCHAR *) "nRF_RxTx", TASK_NRF_STACK,
	       NULL, TASK_NRF_PRIORITY, NULL);
}

