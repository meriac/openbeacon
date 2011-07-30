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
#include "bprotocol.h"
#include "nRF24L01/nRF_HW.h"
#include "nRF24L01/nRF_CMD.h"
#include "nRF24L01/nRF_API.h"
#include "debug_printf.h"
#include "env.h"
#include "rnd.h"

#define DEFAULT_JAM_DENSITY 10
#define TX_COMMAND_RETRIES 3

static BRFPacket rfpkg;
unsigned int rf_sent_broadcast, rf_sent_unicast, rf_rec;
static unsigned char nrf_powerlevel_current, nrf_powerlevel_last;
static unsigned int jam_density_ms;
static const unsigned char broadcast_mac[NRF_MAX_MAC_SIZE] =
  { 'D', 'E', 'C', 'A', 'D' };
static unsigned char jam_mac[NRF_MAX_MAC_SIZE] =
  { 'J', 'A', 'M', 'M', 0 };
static unsigned char wmcu_mac[NRF_MAX_MAC_SIZE] =
  { 'W', 'M', 'C', 'U', 0 };

static void
PtUpdateWmcuId (unsigned char broadcast)
{
  /* update jamming data channel id */
  if (broadcast)
    nRFAPI_SetTxMAC (DEFAULT_DEV, broadcast_mac, sizeof (broadcast_mac));
  else
    {
      jam_mac[sizeof (jam_mac) - 1] = env.e.mcu_id;
      nRFAPI_SetTxMAC (DEFAULT_DEV, jam_mac, sizeof (jam_mac));
    }

  /* update WMCU id for response channel */
  wmcu_mac[sizeof (wmcu_mac) - 1] = env.e.mcu_id;
  nRFAPI_SetRxMAC (DEFAULT_DEV, wmcu_mac, sizeof (wmcu_mac), 1);
}

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
  if (!nRFAPI_Init (DEFAULT_DEV, DEFAULT_CHANNEL, broadcast_mac,
		    sizeof (broadcast_mac), ENABLED_NRF_FEATURES))
    return 0;

  jam_density_ms = DEFAULT_JAM_DENSITY;

  nrf_powerlevel_last = nrf_powerlevel_current = -1;
  PtSetRfPowerLevel (NRF_POWERLEVEL_MAX);

  nRFAPI_SetSizeMac (DEFAULT_DEV, sizeof (wmcu_mac));
  nRFAPI_SetPipeSizeRX (DEFAULT_DEV, 0, sizeof (rfpkg));
  nRFAPI_SetPipeSizeRX (DEFAULT_DEV, 1, sizeof (rfpkg));
  nRFAPI_PipesEnable (DEFAULT_DEV, ERX_P0 | ERX_P1);
  PtUpdateWmcuId (env.e.mcu_id == 0);

  nRFAPI_SetRxMode (DEFAULT_DEV, 0);
  nRFCMD_CE (DEFAULT_DEV, 0);

  return 1;
}

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
  /* update the sequence */
  if (sequence_seed == 0)
    return;

  /* turn on redLED for TX indication */
  vLedSetRed (1);

  /* disable receive mode */
  nRFCMD_CE (DEFAULT_DEV, 0);

  /* wait in case a packet is currently received */
  vTaskDelay (3 / portTICK_RATE_MS);

  /* set TX mode */
  nRFAPI_SetRxMode (DEFAULT_DEV, 0);

  if (pkg->mac == 0xffff)
    rf_sent_broadcast++;
  else
    rf_sent_unicast++;

  pkg->sequence = sequence_seed + (xTaskGetTickCount () / portTICK_RATE_MS);

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
  nRFAPI_TX (DEFAULT_DEV, (unsigned char *) pkg, sizeof (*pkg));

  /* transmit data */
  nRFCMD_CE (DEFAULT_DEV, 1);

  /* wait till packet is transmitted */
  vTaskDelay (3 / portTICK_RATE_MS);

  /* switch to RX mode again */
  nRFAPI_SetRxMode (DEFAULT_DEV, 1);

  /* turn off red TX indication LED */
  vLedSetRed (0);
}

void
PtTransmit (BRFPacket * pkg, unsigned char broadcast)
{
  int i;
  static BRFPacket backup;

  if (TX_COMMAND_RETRIES > 1)
    backup = *pkg;

  if (broadcast)
    PtUpdateWmcuId (pdTRUE);

  for (i = 0; i < TX_COMMAND_RETRIES; i++)
    {
      if (i)
	*pkg = backup;
      vTaskDelay (((RndNumber () % 5) / portTICK_RATE_MS));
      PtInternalTransmit (pkg);
    }

  if (broadcast)
    {
      vTaskDelay ((3 + (RndNumber () % 5) / portTICK_RATE_MS));
      PtUpdateWmcuId (pdFALSE);
    }
}

void
PtSetRfJamDensity (unsigned char milliseconds)
{
  jam_density_ms = milliseconds;
}

unsigned char
PtGetRfJamDensity (void)
{
  return jam_density_ms;
}

static void
vnRFtaskRxTx (void *parameter)
{
  u_int32_t crc, t;
  u_int8_t status;
  portTickType last_ticks = 0, jam_ticks = 0;

  if (!PtInitNRF ())
    return;

  for (;;)
    {
      /* check if TX strength changed */
      if (nrf_powerlevel_current != nrf_powerlevel_last)
	{
	  nRFAPI_SetTxPower (DEFAULT_DEV, nrf_powerlevel_current);
	  nrf_powerlevel_last = nrf_powerlevel_current;
	}

      status = nRFAPI_GetFifoStatus (DEFAULT_DEV);
      /* living in a paranoid world ;-) */
      if (status & FIFO_TX_FULL)
	nRFAPI_FlushTX (DEFAULT_DEV);

      /* check for received packets */
      if ((status & FIFO_RX_FULL) || nRFCMD_WaitRx (0))
	{
	  vLedSetGreen (0);

	  do
	    {
	      /* read packet from nRF chip */
	      nRFCMD_RegReadBuf (DEFAULT_DEV, RD_RX_PLOAD, (unsigned char *) &rfpkg,
				 sizeof (rfpkg));

	      /* adjust byte order and decode */
	      shuffle_tx_byteorder ((unsigned long *) &rfpkg,
				    sizeof (rfpkg) / sizeof (long));
	      xxtea_decode ((long *) &rfpkg, sizeof (rfpkg) / sizeof (long));
	      shuffle_tx_byteorder ((unsigned long *) &rfpkg,
				    sizeof (rfpkg) / sizeof (long));

	      /* verify the crc checksum */
	      crc =
		crc32 ((unsigned char *) &rfpkg,
		       sizeof (rfpkg) - sizeof (rfpkg.crc));
	      if ((crc == PtSwapLong (rfpkg.crc)) &&
		  (rfpkg.wmcu_id == env.e.mcu_id) &&
		  (rfpkg.cmd & RF_PKG_SENT_BY_DIMMER))
		{
		  //debug_printf("dumping received packet:\n");
		  //hex_dump((unsigned char *) &rfpkg, 0, sizeof(rfpkg));
		  b_parse_rfrx_pkg (&rfpkg);
		  rf_rec++;
		}
	    }
	  while ((nRFAPI_GetFifoStatus (DEFAULT_DEV) & FIFO_RX_EMPTY) == 0);

	  vLedSetGreen (1);
	}

      /* transmit current lamp value */
      if (env.e.mcu_id && ((xTaskGetTickCount () - last_ticks) > jam_ticks))
	{
	  memset (&rfpkg, 0, sizeof (rfpkg));
	  rfpkg.cmd = RF_CMD_SET_VALUES;
	  rfpkg.wmcu_id = env.e.mcu_id;
	  rfpkg.mac = 0xffff;	/* send to all MACs */

	  for (t = 0; t < RF_PAYLOAD_SIZE; t++)
	    rfpkg.payload[t] =
	      (last_lamp_val[t * 2] & 0xf) |
	      (last_lamp_val[(t * 2) + 1] << 4);

	  // random delay to avoid collisions
	  PtInternalTransmit (&rfpkg);

	  // prepare next jam transmission
	  last_ticks = xTaskGetTickCount ();
	  jam_ticks =
	    (RndNumber () % (jam_density_ms * 2)) / portTICK_RATE_MS;
	}

      vTaskDelay (5 / portTICK_RATE_MS);

      /* did I already mention the paranoid world thing? */
      nRFAPI_ClearIRQ (DEFAULT_DEV, MASK_IRQ_FLAGS);
    }
}

void
PtInitProtocol (void)
{
  rf_rec = rf_sent_unicast = rf_sent_broadcast = 0;
  xTaskCreate (vnRFtaskRxTx, (signed portCHAR *) "nRF_RxTx", TASK_NRF_STACK,
	       NULL, TASK_NRF_PRIORITY, NULL);
}
