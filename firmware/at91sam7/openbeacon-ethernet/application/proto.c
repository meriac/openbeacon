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
#include <queue.h>
#include <ff.h>
#include "xxtea.h"
#include "led.h"
#include "proto.h"
#include "nRF24L01/nRF_HW.h"
#include "nRF24L01/nRF_CMD.h"
#include "nRF24L01/nRF_API.h"
#include "debug_printf.h"
#include "env.h"
#include "network.h"
#include "rnd.h"

/**********************************************************************/
#define SECTOR_BUFFER_SIZE 1024
/**********************************************************************/
static xQueueHandle xLogfile;
/**********************************************************************/
static unsigned int rf_rec, rf_decrypt, rf_crc_ok;
static unsigned int rf_crc_err, rf_pkt_per_sec, rf_rec_old;
static int pt_debug_level = 0;
static unsigned char nrf_powerlevel_current, nrf_powerlevel_last;
static const unsigned char broadcast_mac[NRF_MAX_MAC_SIZE] = { 0xDE, 0xAD, 0xBE, 0xEF, 42 };

TBeaconEnvelopeLog g_Beacon;

typedef struct
{
  portTickType time;
  u_int32_t id;
} TVisibleTagList;

/**********************************************************************/
#define MAX_TAGS 32
#define MAX_TAG_TRACKING_TIME 20
static TVisibleTagList g_rf_tag_list[MAX_TAGS];
/**********************************************************************/

/**********************************************************************/
void
PtSetDebugLevel (int Level)
{
  pt_debug_level = Level;
}

/**********************************************************************/
void
PtSetRfPowerLevel (unsigned char Level)
{
  nrf_powerlevel_current =
    (Level >= NRF_POWERLEVEL_MAX) ? NRF_POWERLEVEL_MAX : Level;
}

/**********************************************************************/
unsigned char
PtGetRfPowerLevel (void)
{
  return nrf_powerlevel_last;
}

/**********************************************************************/
static inline s_int8_t
PtInitNRF (void)
{
  if (!nRFAPI_Init (DEFAULT_CHANNEL, broadcast_mac,
		    sizeof (broadcast_mac),
		    (FEATURE_EN_ACK_PAY|FEATURE_EN_DYN_ACK|FEATURE_EN_DPL)))
    return 0;

  nrf_powerlevel_last = nrf_powerlevel_current = -1;

  nRFAPI_SetPipeSizeRX (0, 16);
  PtSetRfPowerLevel (0);
  nRFAPI_SetTxPower (0);
  nRFAPI_PipesAck (ERX_P0);
  nRFAPI_SetRxMode (1);

  nRFCMD_CE (1);

  return 1;
}

/**********************************************************************/
void
led_set_rx (bool_t on)
{
  if (on)
    AT91F_PIO_ClearOutput (LED_BEACON_PIO, LED_BEACON_GREEN);
  else
    AT91F_PIO_SetOutput (LED_BEACON_PIO, LED_BEACON_GREEN);
}

/**********************************************************************/
static void
led_set_tx (bool_t on)
{
  if (on)
    AT91F_PIO_ClearOutput (LED_BEACON_PIO, LED_BEACON_RED);
  else
    AT91F_PIO_SetOutput (LED_BEACON_PIO, LED_BEACON_RED);
}

/**********************************************************************/
#define SHUFFLE(a,b)    tmp=g_Beacon.log.byte[a];\
                        g_Beacon.log.byte[a]=g_Beacon.log.byte[b];\
                        g_Beacon.log.byte[b]=tmp;

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

/**********************************************************************/
static inline unsigned short
swapshort (unsigned short src)
{
  return (src >> 8) | (src << 8);
}

/**********************************************************************/
static inline unsigned long
swaplong (unsigned long src)
{
  return (src >> 24) | (src << 24) | ((src >> 8) & 0x0000FF00) | ((src << 8) &
								  0x00FF0000);
}

/**********************************************************************/
static char
put_hex_char (u_int8_t hexnum)
{
  return (hexnum <= 9) ? hexnum + '0' : ((hexnum & 0xF) - 0xA) + 'A';
}

/**********************************************************************/
static void
put_hex_oid (char *hex, u_int16_t oid)
{
  hex[0] = put_hex_char ((oid >> 12) & 0xF);
  hex[1] = put_hex_char ((oid >> 8) & 0xF);
  hex[2] = put_hex_char ((oid >> 4) & 0xF);
  hex[3] = put_hex_char ((oid >> 0) & 0xF);
}

/**********************************************************************/
static void
vnRFtaskRxTx (void *parameter)
{
  int i, first_free;
  u_int8_t status, found;
  u_int16_t crc, oid, id;
  unsigned int delta_t_ms;
  portTickType time, time_old, seconds_since_boot;

  if (!PtInitNRF ())
    while (1)
      {
	vTaskDelay (1000 / portTICK_RATE_MS);
	led_set_tx (1);
	led_set_rx (0);

	vTaskDelay (1000 / portTICK_RATE_MS);
	led_set_tx (0);
	led_set_rx (1);
      }

  led_set_tx (1);

  time_old = xTaskGetTickCount () * portTICK_RATE_MS;

  for (;;)
    {
      /* gather statistics */
      time = xTaskGetTickCount () * portTICK_RATE_MS;
      delta_t_ms = time - time_old;
      seconds_since_boot = time / 1000;

      if (delta_t_ms > 1000)
	{
	  time_old = time;
	  rf_pkt_per_sec = (rf_rec - rf_rec_old) * 1000 / delta_t_ms;
	  rf_rec_old = rf_rec;

	  /* throw out stuff after MAX_TAG_TRACKING_TIME seconds passed */
	  for (i = 0; i < MAX_TAGS; i++)
	    {
	      id = g_rf_tag_list[i].id;

	      if (id
		  && ((seconds_since_boot - g_rf_tag_list[i].time) >=
		      MAX_TAG_TRACKING_TIME))
		{
		  if (pt_debug_level)
		    debug_printf (" * removed tag %u from list[%u]\n", id, i);
		  memset (&g_rf_tag_list[i], 0, sizeof (g_rf_tag_list[0]));

		  memset (&g_Beacon.log, 0, sizeof (g_Beacon.log));
		  strcat ((char *) &g_Beacon.log, "TAG_RMV=");
		  put_hex_oid ((char *) &g_Beacon.log.byte[0x8], id);
		  g_Beacon.log.byte[0xC] = ';';
		  g_Beacon.log.byte[0xD] = '\n';
		  vNetworkSendBeaconToServer ();
		}
	    }
	}

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
	  led_set_rx (1);

	  do
	    {
#ifndef DISABLE_WATCHDOG
	      /* Restart watchdog, has been enabled in Cstartup_SAM7.c */
	      AT91F_WDTRestart (AT91C_BASE_WDTC);
#endif /*DISABLE_WATCHDOG */

	      // storing timestamp into log file queue item
	      g_Beacon.time = swaplong ((u_int32_t) xTaskGetTickCount ());

	      // read packet from nRF chip
	      nRFCMD_RegReadBuf (RD_RX_PLOAD, g_Beacon.log.byte,
				 sizeof (g_Beacon.log));

	      rf_rec++;

	      // posting packet to log file queue
//            xQueueSend (xLogfile, &g_Beacon, 0);

	      // post packet to network via UDP
//            vNetworkSendBeaconToServer ();

	      // adjust byte order and decode
	      shuffle_tx_byteorder ();
	      xxtea_decode ();
	      shuffle_tx_byteorder ();

	      rf_decrypt++;

	      // verify the crc checksum
	      crc =
		env_crc16 (g_Beacon.log.byte,
			   sizeof (g_Beacon.log) - sizeof (u_int16_t));

	      if (swapshort (g_Beacon.log.pkt.crc) != crc)
		rf_crc_err++;
	      else
		{
		  rf_crc_ok++;

		  if (g_Beacon.log.pkt.proto == RFBPROTO_BEACONTRACKER)
		    {
		      oid = swapshort (g_Beacon.log.pkt.oid);
		      if ((pt_debug_level)
			  && (g_Beacon.log.pkt.flags & RFBFLAGS_SENSOR))
			debug_printf ("@%07u BUTTON: %u\n",
				      seconds_since_boot, oid);

		      found = 0;
		      first_free = -1;
		      for (i = 0; i < MAX_TAGS; i++)
			{
			  id = g_rf_tag_list[i].id;

			  if (oid == id)
			    {
			      g_rf_tag_list[i].time = seconds_since_boot;
			      found = 1;
			      memset (&g_Beacon.log, 0,
				      sizeof (g_Beacon.log));
			      strcat ((char *) &g_Beacon.log, "TAG_PNG=");
			      put_hex_oid ((char *) &g_Beacon.log.byte[0x8],
					   id);
			      g_Beacon.log.byte[0xC] = ';';
			      g_Beacon.log.byte[0xD] = '\n';
			      vNetworkSendBeaconToServer ();
			      break;
			    }
			  else if (!id)
			    {
			      if (first_free < 0)
				first_free = i;
			    }
			}
		      if (!found && (first_free >= 0))
			{
			  if (pt_debug_level)
			    debug_printf (" * added Tag [%u] to list[%u]\n",
					  oid, first_free);
			  g_rf_tag_list[first_free].id = oid;
			  g_rf_tag_list[first_free].time = seconds_since_boot;

			  memset (&g_Beacon.log, 0, sizeof (g_Beacon.log));
			  strcat ((char *) &g_Beacon.log, "TAG_ADD=");
			  put_hex_oid ((char *) &g_Beacon.log.byte[0x8], oid);
			  g_Beacon.log.byte[0xC] = ';';
			  g_Beacon.log.byte[0xD] = '\n';
			  vNetworkSendBeaconToServer ();
			}
		    }
		}
	    }
	  while ((nRFAPI_GetFifoStatus () & FIFO_RX_EMPTY) == 0);

	  /* wait in case a packet is currently received */
	  led_set_rx (0);
	}

      /* did I already mention this paranoid world thing? */
      nRFAPI_ClearIRQ (MASK_IRQ_FLAGS);


    }
}

/**********************************************************************/
void
PtStatusRxTx (void)
{
  debug_printf ("\nOpenBeacon Interface Status:\n");
  debug_printf ("\treceived    = %u\n", rf_rec);
  debug_printf ("\treceive rate= %u packets/second\n", rf_pkt_per_sec);

  if (pt_debug_level)
    {
      debug_printf ("\tdecrypted   = %u\n", rf_decrypt);
      debug_printf ("\tpkt crc ok  = %u\n", rf_crc_ok);
      debug_printf ("\tpkt crc err = %u\n", rf_crc_err);
    }
  debug_printf ("\n");
}


/**********************************************************************/
static void
vnRFLogFileFileTask (void *parameter)
{
  UINT written;
  FRESULT res;
  static TBeaconEnvelopeLog data;
  static FIL fil;
  static FATFS fatfs;
  static FILINFO filinfo;
  static char logfile[] = "LOGFILE_.BIN";
  portTickType time, time_old;

  /* delay SD card init by 5 seconds */
  vTaskDelay (5000 / portTICK_RATE_MS);

  /* never fails - data init */
  memset (&fatfs, 0, sizeof (fatfs));
  f_mount (0, &fatfs);

  for (logfile[7] = 'A'; logfile[7] < 'Z'; logfile[7]++)
    switch (res = f_stat (logfile, &filinfo))
      {

      case FR_OK:
	/* found existing logfile, advancing to next possible one */
	debug_printf ("Skipping existing logfile '%s'\n", logfile);
	break;

      case FR_NO_FILE:
      case FR_INVALID_NAME:
	/* opening new file for write access */
	debug_printf ("\nCreating logfile (%s).\n", logfile);
	if (f_open (&fil, logfile, FA_WRITE | FA_CREATE_ALWAYS))
	  debug_printf ("\nfailed to create file\n");
	else
	  {
	    /* Enable Debug output as we were able to open the log file */
	    debug_printf ("OpenBeacon firmware version %s\nreader_id=%i.\n",
			  VERSION, env.e.reader_id);

	    /* Storing clock ticks for flushing cache action */
	    time_old = xTaskGetTickCount ();

	    for (;;)
	      {
		/* query queue for new data with a 500ms timeout */
		if (xQueueReceive (xLogfile, &data, 500 * portTICK_RATE_MS))
		  {
		    if (f_write
			(&fil, &data, sizeof (data), &written)
			|| written != sizeof (data))
		      debug_printf ("\nfailed to write to logfile\n");
		  }

		/* flush file every 10 seconds */
		time = xTaskGetTickCount ();
		if ((time - time_old) >= (10000 * portTICK_RATE_MS))
		  {
		    time_old = time;
		    if (f_sync (&fil))
		      debug_printf ("\nfailed to flush to logfile\n");
		  }
	      }
	  }
	break;

      default:
	debug_printf ("Error (%i) while searching for logfile '%s'\n", res,
		      logfile);
      }

  // error blinking
  while (1)
    {
      led_set_red (1);
      vTaskDelay (250 / portTICK_RATE_MS);

      led_set_red (0);
      vTaskDelay (250 / portTICK_RATE_MS);
    }
}

/**********************************************************************/
void
PtInitProtocol (void)
{
  // turn off LEDs
  AT91F_PIO_CfgOutput (LED_BEACON_PIO, LED_BEACON_MASK);
  AT91F_PIO_SetOutput (LED_BEACON_PIO, LED_BEACON_MASK);

  rf_rec = rf_rec_old = rf_decrypt = 0;
  rf_crc_ok = rf_crc_err = rf_pkt_per_sec = 0;
  memset (&g_rf_tag_list, 0, sizeof (g_rf_tag_list));

  xLogfile =
    xQueueCreate ((SECTOR_BUFFER_SIZE * 2) / sizeof (g_Beacon),
		  sizeof (g_Beacon));

  xTaskCreate (vnRFtaskRxTx, (signed portCHAR *) "nRF_RxTx", TASK_NRF_STACK,
	       NULL, TASK_NRF_PRIORITY, NULL);

  xTaskCreate (vnRFLogFileFileTask, (signed portCHAR *) "nRF_Logfile",
	       TASK_FILE_STACK, NULL, TASK_FILE_PRIORITY, NULL);
}
