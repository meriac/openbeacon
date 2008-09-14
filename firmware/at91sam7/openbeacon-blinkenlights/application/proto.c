/***************************************************************
 *
 * OpenBeacon.org - OpenBeacon dimmer link layer protocol
 *
 * Copyright 2008 Milosch Meriac <meriac@openbeacon.de>
 *                Daniel Mack <daniel@caiaq.de>
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
#include <math.h>
#include <board.h>
#include <beacontypes.h>
#include <USB-CDC.h>
#include "led.h"
#include "xxtea.h"
#include "proto.h"
#include "nRF24L01/nRF_HW.h"
#include "nRF24L01/nRF_CMD.h"
#include "nRF24L01/nRF_API.h"
#include "dimmer.h"
#include "env.h"
#include "update.h"
#include "debug_print.h"

static BRFPacket pkg;
static unsigned long packet_count;
const unsigned char broadcast_mac[NRF_MAX_MAC_SIZE] =
  { 'D', 'E', 'C', 'A', 'D' };

#define BLINK_INTERVAL_MS (50 / portTICK_RATE_MS)

static inline s_int8_t
PtInitNRF (void)
{
  if (!nRFAPI_Init (DEFAULT_CHANNEL, broadcast_mac,
		    sizeof (broadcast_mac), ENABLED_NRF_FEATURES))
    return 0;

  nRFAPI_SetPipeSizeRX (0, sizeof (pkg));
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
  return (src >> 24) |
    (src << 24) | ((src >> 8) & 0x0000FF00) | ((src << 8) & 0x00FF0000);
}

static void
shuffle_tx_byteorder (unsigned long *v, int len)
{
  while (len--)
    {
      *v = swaplong (*v);
      v++;
    }
}

static inline void
sendReply (void)
{
  vTaskDelay (100 / portTICK_RATE_MS);
  pkg.mac = env.e.mac;
  pkg.wmcu_id = env.e.wmcu_id;

  /* mark packet as being sent from an dimmer
   * so it's ignored by other dimmers */
  pkg.cmd |= 0x40;

  /* update crc */
  pkg.crc =
    env_crc16 ((unsigned char *) &pkg, sizeof (pkg) - sizeof (pkg.crc));
  pkg.crc = swapshort (pkg.crc);

  /* encrypt data */
  shuffle_tx_byteorder ((unsigned long *) &pkg, sizeof (pkg) / sizeof (long));
  xxtea_encode ((long *) &pkg, sizeof (pkg) / sizeof (long));
  shuffle_tx_byteorder ((unsigned long *) &pkg, sizeof (pkg) / sizeof (long));

  /* disable RX mode */
  nRFCMD_CE (0);

  /* switch to TX mode */
  nRFAPI_SetRxMode (0);

  /* upload data to nRF24L01 */
  nRFAPI_TX ((unsigned char *) &pkg, sizeof (pkg));

  /* transmit data */
  nRFCMD_CE (1);

  /* wait until packet is transmitted */
  vTaskDelay (10 / portTICK_RATE_MS);

  /* switch to RX mode again */
  nRFAPI_SetRxMode (1);
}

static inline void
bParsePacket (void)
{
  int i, reply;

  /* no MAC set yet? do nothing */
  //if (!env.e.mac)
  //  return;

  /* broadcasts have to have the correct wmcu_id set */
  if (pkg.mac == 0xffff && pkg.wmcu_id != env.e.wmcu_id)
    return;

  /* for all other packets, we want our mac */
  if (pkg.mac != 0xffff && pkg.mac != env.e.mac)
    return;

  /* ignore pakets sent from another dimmer */
  if (pkg.cmd & 0x40)
    return;

  reply = pkg.cmd & 0x80;
  pkg.cmd &= ~0x80;

  switch (pkg.cmd)
    {
    case RF_CMD_SET_VALUES:
      {
	char v;

	if (env.e.lamp_id * 2 >= RF_PAYLOAD_SIZE)
	  break;

	v = pkg.payload[env.e.lamp_id / 2];
	if (env.e.lamp_id & 1)
	  v >>= 4;

	v &= 0xf;

	//DumpStringToUSB ("new lamp val: ");
	//DumpUIntToUSB (v);
	//DumpStringToUSB ("\n\r");

	vUpdateDimmer (v);
	packet_count++;
	break;
      }
    case RF_CMD_SET_LAMP_ID:
      DumpStringToUSB ("new lamp id: ");
      DumpUIntToUSB (pkg.set_lamp_id.id);
      DumpStringToUSB (", wmcu id: ");
      DumpUIntToUSB (pkg.set_lamp_id.wmcu_id);
      DumpStringToUSB ("\n\r");

      env.e.lamp_id = pkg.set_lamp_id.id;
      env.e.wmcu_id = pkg.set_lamp_id.wmcu_id;
      vTaskDelay (100);
      env_store ();

      break;
    case RF_CMD_SET_GAMMA:
      if (pkg.set_gamma.block > 1)
	break;

      for (i = 0; i < 8; i++)
	vSetDimmerGamma (pkg.set_gamma.block * 8 + i, pkg.set_gamma.val[i]);

      DumpStringToUSB ("new gamme table received\n");
      break;
    case RF_CMD_WRITE_CONFIG:
      env_store ();
      break;
    case RF_CMD_SET_JITTER:
      vSetDimmerJitterUS (pkg.set_jitter.jitter);

      DumpStringToUSB ("new jitter: ");
      DumpUIntToUSB (vGetDimmerJitterUS ());
      DumpStringToUSB ("\n");

      break;
    case RF_CMD_SEND_STATISTICS:
      pkg.statistics.emi_pulses = vGetEmiPulses ();
      pkg.statistics.packet_count = packet_count;
      break;
    case RF_CMD_ENTER_UPDATE_MODE:
      if (pkg.payload[0] != 0xDE ||
	  pkg.payload[1] != 0xAD ||
	  pkg.payload[2] != 0xBE ||
	  pkg.payload[3] != 0xEF || pkg.mac == 0xffff)
	break;

      DumpStringToUSB (" ENTERING UPDATE MODE!\n");
      DeviceRevertToUpdateMode ();
    }

  if (reply)
    sendReply ();
}

void
vnRFtaskRx (void *parameter)
{
  u_int16_t crc;
  (void) parameter;
  int DidBlink = 0;
  portTickType Ticks = 0;
  packet_count = 0;

  if (!PtInitNRF ())
    return;

  for (;;)
    {
      if (!nRFCMD_WaitRx (10))
	{
	  nRFAPI_ClearIRQ (MASK_IRQ_FLAGS);
	  continue;
	}

      DumpStringToUSB ("received packet\n");

      do
	{
	  /* read packet from nRF chip */
	  nRFCMD_RegReadBuf (RD_RX_PLOAD, (unsigned char *) &pkg,
			     sizeof (pkg));

	  /* adjust byte order and decode */
	  shuffle_tx_byteorder ((unsigned long *) &pkg,
				sizeof (pkg) / sizeof (long));
	  xxtea_decode ((long *) &pkg, sizeof (pkg) / sizeof (long));
	  shuffle_tx_byteorder ((unsigned long *) &pkg,
				sizeof (pkg) / sizeof (long));

	  /* verify the crc checksum */
	  crc =
	    env_crc16 ((unsigned char *) &pkg,
		       sizeof (pkg) - sizeof (pkg.crc));

	  if (crc != swapshort (pkg.crc))
	    continue;

	  /* valid paket */
	  if (!DidBlink)
	    {
	      vLedSetGreen (1);
	      Ticks = xTaskGetTickCount ();
	      DidBlink = 1;
	    }

	  bParsePacket ();
	}
      while ((nRFAPI_GetFifoStatus () & FIFO_RX_EMPTY) == 0);

      nRFAPI_ClearIRQ (MASK_IRQ_FLAGS);

      if (DidBlink && ((xTaskGetTickCount () - Ticks) > BLINK_INTERVAL_MS))
	{
	  DidBlink = 0;
	  vLedSetGreen (0);
	}
    }				/* for (;;) */
}

void
vInitProtocolLayer (void)
{
  xTaskCreate (vnRFtaskRx, (signed portCHAR *) "nRF_Rx", TASK_NRF_STACK,
	       NULL, TASK_NRF_PRIORITY, NULL);
}
