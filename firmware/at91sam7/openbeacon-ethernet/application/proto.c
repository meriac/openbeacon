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
#include <crc32.h>
#include "led.h"
#include "xxtea.h"
#include "proto.h"
#include "nRF24L01/nRF_HW.h"
#include "nRF24L01/nRF_CMD.h"
#include "nRF24L01/nRF_API.h"
#include "debug_printf.h"
#include "env.h"
#include "rnd.h"

unsigned int rf_sent_broadcast, rf_sent_unicast, rf_rec;
static unsigned char nrf_powerlevel_current, nrf_powerlevel_last;
const unsigned char broadcast_mac[NRF_MAX_MAC_SIZE] = { 1, 2, 3, 2, 1 };

void
PtSetRfPowerLevel (unsigned char Level)
{
  nrf_powerlevel_current =
    (Level >= NRF_POWERLEVEL_MAX) ? NRF_POWERLEVEL_MAX : Level;
}

unsigned char
PtGetRfPowerLevel (void)
{
  return nrf_powerlevel_last;
}

static inline s_int8_t
PtInitNRF (void)
{
  if (!nRFAPI_Init (DEFAULT_CHANNEL, broadcast_mac,
		    sizeof (broadcast_mac), 0))
    return 0;

  nrf_powerlevel_last = nrf_powerlevel_current = -1;

  nRFAPI_SetPipeSizeRX (0, 16);
  PtSetRfPowerLevel (NRF_POWERLEVEL_MAX);
  nRFAPI_SetRxMode (1);

  nRFCMD_CE (1);

  return 1;
}

#if 0

static void
shuffle_tx_byteorder (unsigned long *v, int len)
{
  while (len--)
    {
      *v = PtSwapLong (*v);
      v++;
    }
}

static void
PtInternalTransmit (BRFPacket * pkg)
{
  /* turn on redLED for TX indication */
  vLedSetRed (1);

  /* disable receive mode */
  nRFCMD_CE (0);

  /* wait in case a packet is currently received */
  vTaskDelay (3 / portTICK_RATE_MS);

  /* set TX mode */
  nRFAPI_SetRxMode (0);

  if (pkg->mac == 0xffff)
    rf_sent_broadcast++;
  else
    rf_sent_unicast++;

  pkg->sequence = xTaskGetTickCount ();

  /* update crc */
  pkg->crc =
    PtSwapLong (crc32
		((unsigned char *) pkg, sizeof (*pkg) - sizeof (pkg->crc)));

  /* encrypt the data */
  shuffle_tx_byteorder ((unsigned long *) pkg, sizeof (*pkg) / sizeof (long));
  xxtea_encode ((long *) pkg, sizeof (*pkg) / sizeof (long));
  shuffle_tx_byteorder ((unsigned long *) pkg, sizeof (*pkg) / sizeof (long));

  /* upload data to nRF24L01 */
  //hex_dump((unsigned char *) pkg, 0, sizeof(*pkg));
  nRFAPI_TX ((unsigned char *) pkg, sizeof (*pkg));

  /* transmit data */
  nRFCMD_CE (1);

  /* wait till packet is transmitted */
  vTaskDelay (3 / portTICK_RATE_MS);

  /* switch to RX mode again */
  nRFAPI_SetRxMode (1);

  /* turn off red TX indication LED */
  vLedSetRed (0);
}
#endif
static void
vnRFtaskRxTx (void *parameter)
{
  u_int8_t status;
  static u_int8_t rfpkg[16];

  if (!PtInitNRF ())
    return;

  for (;;)
    {
      /* check if TX strength changed */
      if (nrf_powerlevel_current != nrf_powerlevel_last)
	{
	  nRFAPI_SetTxPower (nrf_powerlevel_current);
	  nrf_powerlevel_last = nrf_powerlevel_current;
	}

      status = nRFAPI_GetFifoStatus ();

      /* living in a paranoid world ;-) */
      if (status & FIFO_TX_FULL)
	nRFAPI_FlushTX ();

      /* check for received packets */
      if ((status & FIFO_RX_FULL) || nRFCMD_WaitRx (100))
	{
	  vLedSetRed (1);

	  do
	    {
	      /* read packet from nRF chip */
	      debug_printf ("RX packet\n");
	      nRFCMD_RegReadBuf (RD_RX_PLOAD, (unsigned char *) &rfpkg,
				 sizeof (rfpkg));
	     /*TODO*/}
	  while ((nRFAPI_GetFifoStatus () & FIFO_RX_EMPTY) == 0);

	  /* wait in case a packet is currently received */
	  vTaskDelay (10 / portTICK_RATE_MS);
	  vLedSetRed (0);
	}

      /* did I already mention this paranoid world thing? */
      nRFAPI_ClearIRQ (MASK_IRQ_FLAGS);

    }
}

void
PtInitProtocol (void)
{
  rf_rec = rf_sent_unicast = rf_sent_broadcast = 0;
  xTaskCreate (vnRFtaskRxTx, (signed portCHAR *) "nRF_RxTx", TASK_NRF_STACK,
	       NULL, TASK_NRF_PRIORITY, NULL);
}
