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
#include <crc32.h>
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

unsigned int packet_count;
unsigned int pings_lost = 0;
unsigned int last_sequence = 0;
unsigned int last_ping_seq = 0;
static unsigned int pt_reset_type = 0;
static portBASE_TYPE pt_dump_registers = 0;
unsigned int debug = 0;

static BRFPacket pkg;
static const unsigned char broadcast_mac[NRF_MAX_MAC_SIZE] =
    { 'D', 'E', 'C', 'A', 'D' };
static unsigned char jam_mac[NRF_MAX_MAC_SIZE] =
    { 'J', 'A', 'M', 'M', 0 };
static unsigned char wmcu_mac[NRF_MAX_MAC_SIZE] =
    { 'W', 'M', 'C', 'U', 0 };

#define BLINK_INTERVAL_MS (50 / portTICK_RATE_MS)

void
PtUpdateWmcuId (unsigned char id)
{
  /* update WMCU id for response channel */
  wmcu_mac[sizeof (wmcu_mac) - 1] = id;
  nRFAPI_SetTxMAC (wmcu_mac, sizeof (wmcu_mac));

  /* update jamming data channel id */
  jam_mac[sizeof (jam_mac) - 1] = id;
  nRFAPI_SetRxMAC (jam_mac, sizeof (jam_mac), 1);
}

static inline s_int8_t
PtInitNRF (void)
{
  if (!nRFAPI_Init (DEFAULT_CHANNEL, broadcast_mac,
		    sizeof (broadcast_mac), ENABLED_NRF_FEATURES))
    return 0;

  nRFAPI_SetSizeMac (sizeof (wmcu_mac));
  nRFAPI_SetPipeSizeRX (0, sizeof (pkg));
  nRFAPI_SetPipeSizeRX (1, sizeof (pkg));
  nRFAPI_PipesEnable (ERX_P0 | ERX_P1);

  PtUpdateWmcuId (env.e.wmcu_id);

  nRFAPI_SetTxPower (3);
  nRFAPI_SetRxMode (1);
  nRFCMD_CE (1);

  return 1;
}

void
PtInitNrfFrontend (int ResetType)
{
  pt_reset_type = ResetType;
}

void
PtDumpNrfRegisters (void)
{
  pt_dump_registers = pdTRUE;
}

static inline void
PtInternalDumpNrfRegisters (void)
{
  unsigned int size;
  unsigned char reg, buf[32], *p;

  DumpStringToUSB ("\nnRF24L01 register dump:\n");
  reg = 0;
  while (((size = nRFCMD_GetRegSize (reg)) > 0) && (reg < 0xFF))
    {
      if (size > sizeof (buf))
	size = sizeof (buf);

      nRFCMD_RegReadBuf (reg, buf, size);
      p = buf;

      // dump single register
      DumpStringToUSB ("  R 0x");
      DumpHexToUSB (reg, 1);
      DumpStringToUSB (":");
      while (size--)
	{
	  DumpStringToUSB (" 0x");
	  DumpHexToUSB (*p++, 1);
	}
      DumpStringToUSB ("\n");

      reg++;
    }
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

inline static void
PtResetDevice (void)
{
  vTaskDelay (100 / portTICK_RATE_MS);
  vTaskSuspendAll ();
  portENTER_CRITICAL ();
  /* endless loop to trigger watchdog reset */
  while (1)
    {
    };
}

static inline void
PtSendReply (void)
{
  pkg.mac = env.e.mac;
  pkg.wmcu_id = env.e.wmcu_id;

  /* mark packet as being sent from an dimmer
   * so it's ignored by other dimmers */
  pkg.cmd |= 0x40;

  /* update crc */
  pkg.crc =
    swaplong (crc32
	      ((unsigned char *) &pkg, sizeof (pkg) - sizeof (pkg.crc)));

  /* encrypt data */
  shuffle_tx_byteorder ((unsigned long *) &pkg, sizeof (pkg) / sizeof (long));
  xxtea_encode ((long *) &pkg, sizeof (pkg) / sizeof (long));
  shuffle_tx_byteorder ((unsigned long *) &pkg, sizeof (pkg) / sizeof (long));

  /* disable RX mode */
  nRFCMD_CE (0);

  vTaskDelay (3 / portTICK_RATE_MS);

  /* switch to TX mode */
  nRFAPI_SetRxMode (0);

  /* upload data to nRF24L01 */
  nRFAPI_TX ((unsigned char *) &pkg, sizeof (pkg));

  /* transmit data */
  nRFCMD_CE (1);

  /* wait until packet is transmitted */
  vTaskDelay (3 / portTICK_RATE_MS);

  /* switch to RX mode again */
  nRFAPI_SetRxMode (1);
}

static inline void
bParsePacket (unsigned char pipe)
{
  int i, reply;
  signed int seq_delta = pkg.sequence - last_sequence;
  char v;
  static char prev_v=-1;

  /* broadcasts have to have the correct wmcu_id set */
  if (pkg.mac == 0xffff && pkg.wmcu_id != env.e.wmcu_id)
    {
      if (debug)
	{
	  DumpStringToUSB ("dropping broadcast packet with wmcu id ");
	  DumpUIntToUSB (pkg.wmcu_id);
	  DumpStringToUSB ("\n");
	}
      return;
    }

  /* for all other packets, we want our mac */
  if (pkg.mac != 0xffff && pkg.mac != env.e.mac)
    {
      if (debug > 2)
	{
	  DumpStringToUSB ("dropping unicast packet for mac ");
	  DumpHexToUSB (pkg.wmcu_id, 1);
	  DumpStringToUSB ("\n");
	}
      return;
    }

  /* ignore pakets sent from another dimmer */
  if (pkg.cmd & 0x40)
    return;

  /* check the sequence */
  if (last_sequence == 0)
    {
      last_sequence = pkg.sequence;
      DumpStringToUSB ("Seeding sequence: ");
      DumpUIntToUSB (last_sequence);
      DumpStringToUSB ("\n");
    }
  else if (!(seq_delta > 0 && seq_delta < (signed int) (1UL << 28)))
    {
      if (debug)
	{
	  DumpStringToUSB ("dropping packet with bogus sequence ");
	  DumpUIntToUSB (pkg.sequence);
	  DumpStringToUSB ("\n");
	}
      return;
    }

  last_sequence = pkg.sequence;
  reply = pkg.cmd & 0x80;
  pkg.cmd &= ~0x80;

  switch (pkg.cmd)
    {
    case RF_CMD_SET_VALUES:
      {
	if (env.e.lamp_id / 2 >= RF_PAYLOAD_SIZE)
	  break;

	v = pkg.payload[env.e.lamp_id / 2];
	if (env.e.lamp_id & 1)
	  v >>= 4;

	v &= 0xf;

	if (debug > 1)
	  {
	    int i;

	    DumpStringToUSB ("got values: ");

	    for (i = 0; i < RF_PAYLOAD_SIZE; i++)
	      {
		DumpHexToUSB (pkg.payload[i], 1);
		DumpStringToUSB (" ");
	      }

	    DumpStringToUSB (" - mine: ");
	    DumpHexToUSB (v, 1);
	    DumpStringToUSB ("\n");
	  }

	if( v != prev_v )
	{
	    vTaskDelay (env.e.dimmer_delay / portTICK_RATE_MS);
	    prev_v = v;
	    vUpdateDimmer (v);
	}
	packet_count++;
	break;
      }
    case RF_CMD_SET_LAMP_ID:
      DumpStringToUSB ("new lamp id: ");
      DumpUIntToUSB (pkg.set_lamp_id.id);
      DumpStringToUSB (", wmcu id: ");
      DumpUIntToUSB (pkg.set_lamp_id.wmcu_id);
      DumpStringToUSB ("\n\r");

      if (pkg.set_lamp_id.id != env.e.lamp_id ||
	  pkg.set_lamp_id.wmcu_id != env.e.wmcu_id)
	{
	  DumpStringToUSB ("storing.\n");
	  env.e.lamp_id = pkg.set_lamp_id.id;
	  env.e.wmcu_id = pkg.set_lamp_id.wmcu_id;
	  PtUpdateWmcuId ( env.e.wmcu_id );
	  vTaskDelay (100);
	  env_store ();
	}
      else
	{
	  DumpStringToUSB ("not storing, values are the same.\n");
	}

      break;
    case RF_CMD_SET_GAMMA:
      if (pkg.set_gamma.block > 1)
	break;

      for (i = 0; i < 8; i++)
	vSetDimmerGamma (pkg.set_gamma.block * 8 + i, pkg.set_gamma.val[i]);

      DumpStringToUSB ("new gamma table received\n");
      break;
    case RF_CMD_WRITE_CONFIG:
      DumpStringToUSB ("writing config.\n");
      env_store ();
      /* wait 10ms the same packet is to arrive soon */
      vTaskDelay (10 / portTICK_RATE_MS);
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
      pkg.statistics.pings_lost = pings_lost;
      pkg.statistics.fw_version = VERSION_INT;
      pkg.statistics.tick_count = (xTaskGetTickCount() / portTICK_RATE_MS) / 1000;
     /*
      pkg.statistics.status =
      	(env.e.wmcu_id & 0xff << 24) |
	(env.e.lamp_id & 0xff << 16) |
	(vGetDimmerOff() & 1);
      */
      break;
    case RF_CMD_SET_DIMMER_DELAY:
      env.e.dimmer_delay = pkg.set_delay.delay;
      DumpStringToUSB ("new delay: ");
      DumpUIntToUSB (env.e.dimmer_delay);
      DumpStringToUSB ("\n");
      break;
    case RF_CMD_SET_DIMMER_CONTROL:
      if (pkg.dimmer_control.off)
	DumpStringToUSB ("forcing dimmer off.\n");
      else
	DumpStringToUSB ("dimmer operating.\n");
      vSetDimmerOff (pkg.dimmer_control.off);
      break;
    case RF_CMD_PING:
      if (last_ping_seq == 0 || last_ping_seq > pkg.ping.sequence)
	{
	  last_ping_seq = pkg.ping.sequence;
	  break;
	}
      pings_lost += pkg.ping.sequence - last_ping_seq - 1;
      break;
    case RF_CMD_RESET:
      {
	PtResetDevice ();
	break;
      }
    case RF_CMD_ENTER_UPDATE_MODE:
      if (pkg.payload[0] != 0xDE ||
	  pkg.payload[1] != 0xAD ||
	  pkg.payload[2] != 0xBE ||
	  pkg.payload[3] != 0xEF || pkg.mac == 0xffff)
	break;
      DumpStringToUSB (" ENTERING UPDATE MODE!\n");
      DeviceRevertToUpdateMode ();
      break;
    }

  if (reply)
    PtSendReply ();
}

static void
vnRFtaskRxTx (void *parameter)
{
  u_int32_t crc;
  (void) parameter;
  int DidBlink = 0;
  unsigned char status, pipe;
  portTickType Ticks = 0;
  packet_count = 0;

  if (!PtInitNRF ())
    return;

  for (;;)
    {
      /* reset RF interface if needed */
      if (pt_reset_type)
	{
	  nRFCMD_CE (0);
	  vLedSetGreen (1);
	  vTaskDelay (10 / portTICK_RATE_MS);
	  switch (pt_reset_type)
	    {
	    case PTINITNRFFRONTEND_RESETFIFO:
	      nRFAPI_FlushRX ();
	      nRFAPI_FlushTX ();
	      nRFAPI_ClearIRQ (MASK_IRQ_FLAGS);
	      nRFAPI_SetRxMode (1);
	      break;

	    case PTINITNRFFRONTEND_INIT:
	      PtInitNRF ();
	      break;

	    }
	  vTaskDelay (10 / portTICK_RATE_MS);
	  nRFCMD_CE (1);
	  vLedSetGreen (0);
	  pt_reset_type = 0;
	}

      /* dump RF interface registers if needed */
      if (pt_dump_registers)
	{
	  PtInternalDumpNrfRegisters ();
	  pt_dump_registers = pdFALSE;
	}
      status = nRFAPI_GetFifoStatus ();
      /* living in a paranoid world ;-) */
      if (status & FIFO_TX_FULL)
	nRFAPI_FlushTX ();

      if ((status & FIFO_RX_FULL) || nRFCMD_WaitRx (10))
	do
	  {
	    /* find out which pipe got the packet */
	    pipe = (nRFAPI_GetStatus () >> 1) & 0x07;

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
	      crc32 ((unsigned char *) &pkg, sizeof (pkg) - sizeof (pkg.crc));

	    if (crc == swaplong (pkg.crc))
	      {
		/* valid paket */
		if (!DidBlink)
		  {
		    vLedSetGreen (1);
		    Ticks = xTaskGetTickCount ();
		    DidBlink = 1;
		  }
		bParsePacket (pipe);
	      }
	    else if (debug)
	      {
		DumpStringToUSB ("invalid CRC! ");
		DumpHexToUSB (crc, 4);
		DumpStringToUSB (" != ");
		DumpHexToUSB (pkg.crc, 4);
		DumpStringToUSB ("\n");
	      }
	  }
	while ((nRFAPI_GetFifoStatus () & FIFO_RX_EMPTY) == 0);

      /* did I already mention the paranoid world thing? */
      nRFAPI_ClearIRQ (MASK_IRQ_FLAGS);

      if (DidBlink && ((xTaskGetTickCount () - Ticks) > BLINK_INTERVAL_MS))
	{
	  DidBlink = 0;
	  vLedSetGreen (0);
	}
    }				/* for (;;) */
}

void
vInitProtocolLayer (unsigned char wmcu_id)
{
  xTaskCreate (vnRFtaskRxTx, (signed portCHAR *) "nRF_RxTx",
	       TASK_NRF_STACK, NULL, TASK_NRF_PRIORITY, NULL);
}

