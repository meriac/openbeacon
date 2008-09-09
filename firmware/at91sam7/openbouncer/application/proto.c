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
#if 0
#include "xxtea.h"
#endif
#include "proto.h"
#include "nRF24L01/nRF_HW.h"
#include "nRF24L01/nRF_CMD.h"
#include "nRF24L01/nRF_API.h"

const unsigned char broadcast_mac[NRF_MAX_MAC_SIZE] = { 1, 2, 3, 2, 1 };
TBouncerEnvelope g_Bouncer;

#if 0
/**********************************************************************/
#define SHUFFLE(a,b)    tmp=g_Bouncer.datab[a];\
                        g_Bouncer.datab[a]=g_Bouncer.datab[b];\
                        g_Bouncer.datab[b]=tmp;

/**********************************************************************/
void RAMFUNC
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
#endif

static inline s_int8_t
PtInitNRF (void)
{
  if (!nRFAPI_Init
      (DEFAULT_CHANNEL, broadcast_mac, sizeof (broadcast_mac),
       ENABLED_NRF_FEATURES))
    return 0;

  nRFAPI_SetPipeSizeRX (0, 16);
  nRFAPI_SetTxPower (3);
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

static inline unsigned char
HexChar (unsigned char nibble)
{
  return nibble + ((nibble < 0x0A) ? '0' : ('A' - 0xA));
}

static void
DumpBufferToUSB (unsigned char *buffer, int len)
{
  int i;

  for (i = 0; i < len; i++)
    {
      vUSBSendByte (HexChar (*buffer >> 4));
      vUSBSendByte (HexChar (*buffer++ & 0xf));
    }
}

static inline void
sendReply (void)
{
  /* disable RX mode */
  nRFCMD_CE (0);

  vTaskDelay (100 / portTICK_RATE_MS);

  memset (&g_Bouncer, 0, sizeof (g_Bouncer));
  g_Bouncer.hdr.version = BOUNCERPKT_VERSION;
  g_Bouncer.hdr.command = BOUNCERPKT_CMD_CHALLENGE_SETUP;

  /* encrypt data */
#if 0
  shuffle_tx_byteorder ();
  xxtea_encode ();
  shuffle_tx_byteorder ();
#endif

  /* switch to TX mode */
  nRFAPI_SetRxMode (0);

  /* upload data to nRF24L01 */
  nRFAPI_TX ((unsigned char *) &g_Bouncer, sizeof (g_Bouncer));

  /* transmit data */
  nRFCMD_CE (1);

  /* wait until packet is transmitted */
  vTaskDelay (10 / portTICK_RATE_MS);

  /* switch to RX mode again */
  nRFAPI_SetRxMode (1);
}

void
vnRFtaskRx (void *parameter)
{
  (void) parameter;

  if (!PtInitNRF ())
    return;

  DumpStringToUSB ("INFO: Receiver active");

  for (;;)
    {
      if (nRFCMD_WaitRx (10))
	{
	  vLedSetRed (1);

	  do
	    {
	      // read packet from nRF chip
	      nRFCMD_RegReadBuf (RD_RX_PLOAD, (unsigned char *) &g_Bouncer,
				 sizeof (g_Bouncer));

	      // adjust byte order and decode
#if 0
	      shuffle_tx_byteorder ();
	      xxtea_decode ();
	      shuffle_tx_byteorder ();
#endif

	      DumpStringToUSB ("RX: ");
	      DumpBufferToUSB ((unsigned char *) &g_Bouncer,
			       sizeof (g_Bouncer));
	      DumpStringToUSB ("\n\r");
	    }
	  while ((nRFAPI_GetFifoStatus () & FIFO_RX_EMPTY) == 0);
	  vLedSetGreen (1);
	  sendReply ();
	  vLedSetGreen (0);

	  vLedSetRed (0);
	  nRFAPI_GetFifoStatus ();
	}
      nRFAPI_ClearIRQ (MASK_IRQ_FLAGS);
    }
}

void
vInitProtocolLayer (void)
{
  xTaskCreate (vnRFtaskRx, (signed portCHAR *) "nRF_Rx", TASK_NRF_STACK,
	       NULL, TASK_NRF_PRIORITY, NULL);
}
