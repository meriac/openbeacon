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
#include "led.h"
#include "xxtea.h"
#include "proto.h"
#include "nRF24L01/nRF_HW.h"
#include "nRF24L01/nRF_CMD.h"
#include "nRF24L01/nRF_API.h"
#include "debug_printf.h"
#include "env.h"

const unsigned char broadcast_mac[NRF_MAX_MAC_SIZE] = { 'D', 'E', 'C', 'A', 'D' };
static BRFPacket rxpkg;

static inline s_int8_t
PtInitNRF (void)
{
  if (!nRFAPI_Init(DEFAULT_CHANNEL, broadcast_mac,
  	sizeof (broadcast_mac), ENABLED_NRF_FEATURES))
    return 0;

  nRFAPI_SetPipeSizeRX (0, sizeof(rxpkg));
  nRFAPI_SetTxPower (3);
  nRFAPI_SetRxMode (0);
  nRFCMD_CE (0);

  return 1;
}

void
shuffle_tx_byteorder (unsigned long *v, int len)
{
  while(len--) {
    *v = swaplong(*v);
    v++;
  }
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

void vnRFTransmitPacket(BRFPacket *pkg)
{
  unsigned short crc;

  /* turn on redLED for TX indication */
  vLedSetRed (1);

  /* disable receive mode */
  nRFCMD_CE (0);

  /* set TX mode */
  nRFAPI_SetRxMode (0);

  /* update crc */
  crc = crc16 ((unsigned char *) pkg, sizeof(*pkg) - sizeof(pkg->crc));
  pkg->crc = swapshort (crc);

  /* encrypt the data */
  shuffle_tx_byteorder ((unsigned long *) pkg, sizeof(*pkg) / sizeof(long));
  xxtea_encode ((long *) pkg, sizeof(*pkg) / sizeof(long));
  shuffle_tx_byteorder ((unsigned long *) pkg, sizeof(*pkg) / sizeof(long));

  /* upload data to nRF24L01 */
  //hex_dump((unsigned char *) pkg, 0, sizeof(*pkg));
  nRFAPI_TX ((unsigned char *) pkg, sizeof(*pkg));

  /* transmit data */
  nRFCMD_CE (1);

  /* wait till packet is transmitted */
  vTaskDelay (10 / portTICK_RATE_MS);

  /* switch to RX mode again */
  nRFAPI_SetRxMode (1);

  /* turn off red TX indication LED */
  vLedSetRed (0);

}

void
vnRFtaskRx (void *parameter)
{
  u_int16_t crc;

  if (!PtInitNRF ())
    return;

  /* FIXME!!! THIS IS RACY AND NEEDS A LOCK!!! */

  for (;;)
    {
      if (nRFCMD_WaitRx (100))
	{
	  vLedSetRed (1);

	  do
	    {
	      /* read packet from nRF chip */
	      nRFCMD_RegReadBuf (RD_RX_PLOAD, (unsigned char *) &rxpkg, sizeof(rxpkg));
	      vLedSetRed (0);

	      /* adjust byte order and decode */
	      shuffle_tx_byteorder ((unsigned long *) & rxpkg, sizeof(rxpkg) / sizeof(long));
	      xxtea_decode ((long *) &rxpkg, sizeof(rxpkg) / sizeof(long));
	      shuffle_tx_byteorder ((unsigned long *) & rxpkg, sizeof(rxpkg) / sizeof(long));

	      /* verify the crc checksum */
	      crc = crc16 ((unsigned char *) &rxpkg, sizeof(rxpkg) - sizeof(rxpkg.crc));
	     
	      /* sort out packets from other domains */
	      if (rxpkg.wmcu_id != env.e.mcu_id)
	        continue;

	      /* require packet to be sent from an dimmer */
	      if (~rxpkg.cmd & 0x40)
	        continue;
	      
	      debug_printf("dumping received packet:\n");
	      hex_dump((unsigned char *) &rxpkg, 0, sizeof(rxpkg));
	    }
	  while ((nRFAPI_GetFifoStatus () & FIFO_RX_EMPTY) == 0);

	}
      nRFAPI_ClearIRQ (MASK_IRQ_FLAGS);

      /* schedule */
      vTaskDelay (100 / portTICK_RATE_MS);
    }
}

void
vInitProtocolLayer (void)
{
  xTaskCreate (vnRFtaskRx, (signed portCHAR *) "nRF_Rx", TASK_NRF_STACK,
	       NULL, TASK_NRF_PRIORITY, NULL);
}
