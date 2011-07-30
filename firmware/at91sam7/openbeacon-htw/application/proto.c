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
#include <USB-CDC.h>
#include "led.h"
#include "xxtea.h"
#include "proto.h"
#include "nRF24L01/nRF_HW.h"
#include "nRF24L01/nRF_CMD.h"
#include "nRF24L01/nRF_API.h"

const unsigned char broadcast_mac[NRF_MAX_MAC_SIZE] = { 1, 2, 3, 2, 1 };

TBeaconEnvelope g_Beacon;

#define ANNOUNCEMENT_TX_POWER 3

/**********************************************************************/
#define SHUFFLE(a,b)    tmp=g_Beacon.byte[a];\
                        g_Beacon.byte[a]=g_Beacon.byte[b];\
                        g_Beacon.byte[b]=tmp;

/**********************************************************************/
void
shuffle_tx_byteorder (void)
{
  unsigned char tmp;

  SHUFFLE (0 + 0, 3 + 0);
  SHUFFLE (1 + 0, 2 + 0);
  SHUFFLE (0 + 4, 3 + 4);
  SHUFFLE (1 + 4, 2 + 4);
  SHUFFLE (0 + 8, 3 + 8);
  SHUFFLE (1 + 8, 2 + 8);
  SHUFFLE (0 + 12, 3 + 12);
  SHUFFLE (1 + 12, 2 + 12);
}

static inline s_int8_t
PtInitNRF (void)
{
  if (!nRFAPI_Init
      (DEFAULT_DEV, DEFAULT_CHANNEL, broadcast_mac, sizeof (broadcast_mac), 0))
    return 0;

  nRFAPI_SetPipeSizeRX (DEFAULT_DEV, 0, 16);
  nRFAPI_SetTxPower (DEFAULT_DEV, ANNOUNCEMENT_TX_POWER);
  nRFAPI_SetRxMode (DEFAULT_DEV, 1);
  nRFCMD_CE (DEFAULT_DEV, 1);

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
  return (src >> 24) | (src << 24) | ((src >> 8) & 0x0000FF00) | ((src << 8) &
								  0x00FF0000);
}

static inline short
crc16 (const unsigned char *buffer, int size)
{
  unsigned short crc = 0xFFFF;

  if (buffer && size)
    while (size--)
      {
	crc = (crc >> 8) | (crc << 8);
	crc ^= *buffer++;
	crc ^= ((unsigned char) crc) >> 4;
	crc ^= crc << 12;
	crc ^= (crc & 0xFF) << 5;
      }

  return crc;
}

static inline void
DumpUIntToUSB (unsigned int data)
{
  int i = 0;
  unsigned char buffer[10], *p = &buffer[sizeof (buffer)];

  do
    {
      *--p = '0' + (unsigned char) (data % 10);
      data /= 10;
      i++;
    }
  while (data);

  while (i--)
    vUSBSendByte (*p++);
}

static inline void
DumpStringToUSB (char *text)
{
  unsigned char data;

  if (text)
    while ((data = *text++) != 0)
      vUSBSendByte (data);
}

void
vnRFtaskRx (void *parameter)
{
  //u_int16_t crc;
  (void) parameter;

  if (!PtInitNRF ())
    return;

  for (;;)
    {
      if (nRFCMD_WaitRx (10))
	{
	  // turn on red LED
	  vLedSetRed (1);

	  do
	    {
	      // read packet from nRF chip
	      nRFCMD_RegReadBuf (DEFAULT_DEV, RD_RX_PLOAD, g_Beacon.byte,
				 sizeof (g_Beacon));


	      // turn off green LED
	      vLedSetGreen (0);
	      u_int8_t i = 0;	// counter variable

	      // must be 4 Bytes, because this is gonna be the IP4-SourceAdress
	      vUSBSendByte (0x00);
	      vUSBSendByte (0xff);
	      vUSBSendByte (0x00);
	      vUSBSendByte (0xff);

	      // dump out all received bytes
	      for (i = 0; i < sizeof (g_Beacon); i++)
		{
		  vUSBSendByte (g_Beacon.byte[i]);
		}

	      // turn on green LED
	      vLedSetGreen (1);

	    }
	  while ((nRFAPI_GetFifoStatus (DEFAULT_DEV) & FIFO_RX_EMPTY) == 0);

	  // turn off red LED
	  vLedSetRed (0);
	}
      nRFAPI_ClearIRQ (DEFAULT_DEV, MASK_IRQ_FLAGS);
    }
}

void
vInitProtocolLayer (void)
{
  xTaskCreate (vnRFtaskRx, (signed portCHAR *) "nRF_Rx", TASK_NRF_STACK,
	       NULL, TASK_NRF_PRIORITY, NULL);
}
