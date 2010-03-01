/***************************************************************
 *
 * OpenBeacon.org - OpenBeacon WIFI driver
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
#include <led.h>
#include <beacontypes.h>
#include <env.h>
#include <USB-CDC.h>
#include <rnd.h>
#include <nRF24L01/nRF_HW.h>
#include <nRF24L01/nRF_CMD.h>
#include <nRF24L01/nRF_API.h>

#include "wifi.h"
#include "xxtea.h"

#define UART_QUEUE_SIZE 1024
#define WLAN_BAUDRATE_FACTORY	9600
#define ANNOUNCE_INTERVAL_TICKS	(500 / portTICK_RATE_MS)

#define ERROR_NO_NRF			(3UL)

// set broadcast mac
static unsigned int do_reset = 0;
static const unsigned char broadcast_mac[NRF_MAX_MAC_SIZE] =
  { 1, 2, 3, 2, 1 };
static xQueueHandle wifi_queue_rx;
TBeaconEnvelope g_Beacon;

void
wifi_isr_handler (void)
{
  u_int8_t data;
  portBASE_TYPE woken = pdFALSE;

  // on RX character
  while (AT91C_BASE_US0->US_CSR & AT91C_US_RXRDY)
    {
      data = (unsigned char) (AT91C_BASE_US0->US_RHR);
      xQueueSendFromISR (wifi_queue_rx, &data, &woken);
    }

  // on TX character
/*  while (AT91C_BASE_US0->US_CSR & AT91C_US_TXRDY)
    {

    }*/

  // ack IRQ
  AT91C_BASE_US0->US_CR = AT91C_US_RSTSTA;
  AT91F_AIC_AcknowledgeIt ();

  // wake up ?
  if (woken)
    portYIELD_FROM_ISR ();
}

void __attribute__ ((naked)) wifi_isr (void)
{
  portSAVE_CONTEXT ();
  wifi_isr_handler ();
  portRESTORE_CONTEXT ();
}

static int
wifi_set_baudrate (unsigned int baud)
{
  unsigned int fp, cd;

  switch (baud)
    {
    case 1843200:
      fp = 5;
      break;
    case 921600:
      fp = 2;
      break;
    case 460800:
      fp = 4;
      break;
    default:
      fp = 0;
    }

  cd = (MCK / (16 * baud)) & 0xFFFF;
  AT91C_BASE_US0->US_BRGR = cd | ((fp & 0x7) << 16);

  return MCK / ((16 * cd) + (2 * fp));
}

static void
wifi_reset (void)
{
  // reset WIFI
  AT91F_PIO_SetOutput (WLAN_PIO, WLAN_ADHOC);
  AT91F_PIO_ClearOutput (WLAN_PIO, WLAN_RESET | WLAN_WAKE);
  vTaskDelay (100 / portTICK_RATE_MS);

  // remove reset line
  AT91F_PIO_SetOutput (WLAN_PIO, WLAN_RESET | WLAN_WAKE);

  // tickle On button for WLAN module
  vTaskDelay (10 / portTICK_RATE_MS);
  AT91F_PIO_ClearOutput (WLAN_PIO, WLAN_WAKE);
}

static void
wifi_reset_settings (int backtofactory)
{
  int i;

  wifi_reset ();
  wifi_set_baudrate (backtofactory ? WLAN_BAUDRATE_FACTORY : WLAN_BAUDRATE);
  i = backtofactory ? 10 : 5;

  while (i--)
    {
      vLedSetRed (i & 1);
      vTaskDelay (1000 / portTICK_RATE_MS);

      if (i & 1)
	AT91F_PIO_SetOutput (WLAN_PIO, WLAN_ADHOC);
      else
	AT91F_PIO_ClearOutput (WLAN_PIO, WLAN_ADHOC);
    }

  // wait
  vTaskDelay (1000 / portTICK_RATE_MS);
  // reset module
  AT91F_PIO_ClearOutput (WLAN_PIO, WLAN_RESET | WLAN_WAKE | WLAN_ADHOC);
  vTaskDelay (100 / portTICK_RATE_MS);
  // remove reset line
  AT91F_PIO_SetOutput (WLAN_PIO, WLAN_RESET | WLAN_WAKE);
  // tickle On button for WLAN module
  vTaskDelay (10 / portTICK_RATE_MS);
  AT91F_PIO_ClearOutput (WLAN_PIO, WLAN_WAKE);
}

static void
wifi_stop_blinking (unsigned long error_code)
{
  unsigned long t;

  while (1)
    {
      for (t = 0; t < error_code; t++)
	{
	  vLedSetGreen (1);
	  vLedSetRed (1);
	  vTaskDelay (100 / portTICK_RATE_MS);
	  vLedSetGreen (0);
	  vLedSetRed (0);
	  vTaskDelay (500 / portTICK_RATE_MS);
	}
      vTaskDelay (1000 / portTICK_RATE_MS);
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
  return (src >> 24) | (src << 24) | ((src >> 8) & 0x0000FF00) | ((src << 8) &
								  0x00FF0000);
}

/**********************************************************************/
#define SHUFFLE(a,b)    tmp=g_Beacon.byte[a];\
                        g_Beacon.byte[a]=g_Beacon.byte[b];\
                        g_Beacon.byte[b]=tmp;

/**********************************************************************/
static void
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

static inline void
wifi_tx (unsigned char power)
{
  /* update crc */
  g_Beacon.pkt.crc = swapshort (env_crc16 (g_Beacon.byte,
					   sizeof (g_Beacon) -
					   sizeof (u_int16_t)));
  /* encrypt data */
  shuffle_tx_byteorder ();
  xxtea_encode ();
  shuffle_tx_byteorder ();

  vLedSetGreen (1);

  /* disable RX mode */
  nRFCMD_CE (0);
  vTaskDelay (2 / portTICK_RATE_MS);

  /* switch to TX mode */
  nRFAPI_SetRxMode (0);

  /* set TX power */
  nRFAPI_SetTxPower (power);

  /* upload data to nRF24L01 */
  nRFAPI_TX (g_Beacon.byte, sizeof (g_Beacon));

  /* transmit data */
  nRFCMD_CE (1);

  /* wait until packet is transmitted */
  vTaskDelay (2 / portTICK_RATE_MS);

  /* switch to RX mode again */
  nRFAPI_SetRxMode (1);

  vLedSetGreen (0);
}

static void
wifi_reader_command (TBeaconReaderCommand * cmd)
{
  u_int8_t res = READ_RES__OK;
  
  cmd->data = 0;

  switch (cmd->opcode)
    {
    case READER_CMD_NOP:
      break;
    case READER_CMD_RESET:
      do_reset = 1;
      break;
    case READER_CMD_RESET_CONFIG:
      wifi_reset_settings (0);
      break;
    case READER_CMD_RESET_FACTORY:
      wifi_reset_settings (1);
      break;
    default:
      res = READ_RES__UNKNOWN_CMD;
    }

  cmd->uptime = swaplong(xTaskGetTickCount ());
  cmd->res = res;
}

static void
wifi_task_nrf (void *parameter)
{
  (void) parameter;
  u_int16_t crc;
  unsigned char power = 0;
  portTickType t, Ticks = 0;

  if (nRFAPI_Init (81, broadcast_mac, sizeof (broadcast_mac), 0))
    {
      nRFAPI_SetPipeSizeRX (0, 16);
      nRFAPI_SetTxPower (3);
      nRFAPI_SetRxMode (1);
      nRFCMD_CE (1);

      Ticks = xTaskGetTickCount ();

      while (1)
	{
	  if (nRFCMD_WaitRx (100))
	    {
	      vLedSetRed (1);
	      do
		{
		  // read packet from nRF chip
		  nRFCMD_RegReadBuf (RD_RX_PLOAD, g_Beacon.byte,
				     sizeof (g_Beacon));
		  // adjust byte order and decode
		  shuffle_tx_byteorder ();
		  xxtea_decode ();
		  shuffle_tx_byteorder ();
		  // verify the crc checksum
		  crc = env_crc16 (g_Beacon.byte,
				   sizeof (g_Beacon) - sizeof (u_int16_t));
		  if (swapshort (g_Beacon.pkt.crc) == crc)
		    switch (g_Beacon.pkt.proto)
		      {
		      case RFBPROTO_READER_COMMAND:
			if ( ((g_Beacon.pkt.flags & RFBFLAGS_ACK) == 0)
			  &&(swapshort (g_Beacon.pkt.oid) == env.e.reader_id)
			  )
			  {
			    wifi_reader_command (&g_Beacon.pkt.p.reader_command);
			    g_Beacon.pkt.flags |= RFBFLAGS_ACK;
			    wifi_tx (3);
			  }
			break;
		      }

		}
	      while ((nRFAPI_GetFifoStatus () & FIFO_RX_EMPTY) == 0);
	      vLedSetRed (0);
	    }

	  nRFAPI_ClearIRQ (MASK_IRQ_FLAGS);

	  if ((t = xTaskGetTickCount ()) >= Ticks)
	    {
	      Ticks = t + (ANNOUNCE_INTERVAL_TICKS / 2)
		+ (RndNumber () % (ANNOUNCE_INTERVAL_TICKS / 2));

	      bzero(&g_Beacon,sizeof(g_Beacon));

	      g_Beacon.pkt.oid = swapshort(env.e.reader_id);
	      g_Beacon.pkt.proto = RFBPROTO_READER_ANNOUNCE;
	      g_Beacon.pkt.p.reader_announce.strength = (power++) & 3;
	      g_Beacon.pkt.p.reader_announce.uptime = swaplong(xTaskGetTickCount ());
	      g_Beacon.pkt.p.reader_announce.ip = 0; // fixme
	      wifi_tx (g_Beacon.pkt.p.reader_announce.strength);
	    }
	}
    }

  wifi_stop_blinking (ERROR_NO_NRF);
}

static void
wifi_task_uart (void *parameter)
{
  (void) parameter;
  char data;

  // enable UART0
  AT91C_BASE_US0->US_CR = AT91C_US_RXEN | AT91C_US_TXEN;
  // enable IRQ
  AT91F_AIC_ConfigureIt (AT91C_ID_US0, 4, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL,
			 wifi_isr);
  AT91C_BASE_US0->US_IER = AT91C_US_RXRDY;	//|AT91C_US_ENDRX;//|AT91C_US_TXRDY|AT91C_US_ENDRX|AT91C_US_ENDTX;
  AT91F_AIC_EnableIt (AT91C_ID_US0);

  // remove reset line
  AT91F_PIO_SetOutput (WLAN_PIO, WLAN_RESET | WLAN_WAKE);
  // tickle On button for WLAN module
  vTaskDelay (100 / portTICK_RATE_MS);
  AT91F_PIO_ClearOutput (WLAN_PIO, WLAN_WAKE);

  for (;;)
    {
      if (xQueueReceive (wifi_queue_rx, &data, (portTickType) 0))
	vUSBSendByte (data);
      else if (vUSBRecvByte (&data, sizeof (data), (portTickType) 0))
	AT91C_BASE_US0->US_THR = data;
      else
	vTaskDelay (10 / portTICK_RATE_MS);

      if (!do_reset)
	AT91F_WDTRestart (AT91C_BASE_WDTC);
    }
}

void
wifi_init (void)
{
  // configure UART peripheral
  AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_US0);
  AT91F_PIO_CfgPeriph (WLAN_PIO,
		       (WLAN_RXD | WLAN_TXD | WLAN_RTS | WLAN_CTS), 0);
  // configure IOs
  AT91F_PIO_CfgOutput (WLAN_PIO, WLAN_ADHOC | WLAN_RESET | WLAN_WAKE);
  AT91F_PIO_ClearOutput (WLAN_PIO, WLAN_RESET | WLAN_ADHOC | WLAN_WAKE);
  // Standard Asynchronous Mode : 8 bits , 1 stop , no parity
  AT91C_BASE_US0->US_MR = AT91C_US_ASYNC_MODE;
  wifi_set_baudrate (WLAN_BAUDRATE);
  // no IRQs
  AT91C_BASE_US0->US_IDR = 0xFFFF;
  // reset and disable UART0
  AT91C_BASE_US0->US_CR =
    AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RXDIS | AT91C_US_TXDIS;
  // receiver time-out (20 bit periods)
  AT91C_BASE_US0->US_RTOR = 20;
  // transmitter timeguard (disabled)
  AT91C_BASE_US0->US_TTGR = 0;
  // FI over DI Ratio Value (disabled)
  AT91C_BASE_US0->US_FIDI = 0;
  // IrDA Filter value (disabled)
  AT91C_BASE_US0->US_IF = 0;
  // create UART queue with minimum buffer size
  wifi_queue_rx = xQueueCreate (UART_QUEUE_SIZE,
				(unsigned portCHAR) sizeof (signed portCHAR));
  xTaskCreate (wifi_task_uart,
	       (signed portCHAR *) "wifi_uart",
	       TASK_UART_STACK, NULL, TASK_UART_PRIORITY, NULL);
  xTaskCreate (wifi_task_nrf,
	       (signed portCHAR *) "wifi_nrf",
	       TASK_NRF_STACK, NULL, TASK_NRF_PRIORITY, NULL);
}
