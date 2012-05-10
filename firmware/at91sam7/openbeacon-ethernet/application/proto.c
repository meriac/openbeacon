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
#define LED_ON_RX_BLINK_DURATION_MS 100
#define DUPLICATES_BACKLOG_SIZE 8
/**********************************************************************/
static xQueueHandle xLogfile;
/**********************************************************************/
static u_int32_t duplicate_backlog[DUPLICATES_BACKLOG_SIZE];
static u_int32_t duplicate_backlog_pos, rf_duplicates[NRFCMD_DEVICES];
static u_int32_t rf_lastblink[NRFCMD_DEVICES];
static u_int32_t rf_decrypt[NRFCMD_DEVICES], rf_crc_ok[NRFCMD_DEVICES];
static u_int32_t rf_crc_err[NRFCMD_DEVICES], rf_pkt_per_sec[NRFCMD_DEVICES];
static u_int32_t rf_rec[NRFCMD_DEVICES], rf_rec_old[NRFCMD_DEVICES];
static int pt_debug_level = 0;
static u_int8_t nrf_powerlevel_current, nrf_powerlevel_last;
static const u_int8_t broadcast_mac[NRF_MAX_MAC_SIZE] = { 1, 2, 3, 2, 1 };

TBeaconLogSighting g_Beacon;

/**********************************************************************/
void
PtSetDebugLevel (int Level)
{
	pt_debug_level = Level;
}

/**********************************************************************/
int
PtGetDebugLevel (void)
{
	return pt_debug_level;
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

static inline s_int8_t
PtInitChannel (u_int8_t device)
{
	if (!nRFAPI_Init
		(device, DEFAULT_CHANNEL, broadcast_mac, sizeof (broadcast_mac), 0))
		return 0;

	nRFAPI_SetPipeSizeRX (device, 0, 16);
	nRFAPI_SetRxMode (device, 1);
	nRFCMD_CE (device, 1);
	return 1;
}

/**********************************************************************/
static inline s_int8_t
PtInitNRF (void)
{
	PtSetRfPowerLevel (NRF_POWERLEVEL_MAX);
	nrf_powerlevel_last = nrf_powerlevel_current = -1;

	if (!(PtInitChannel (NRFCMD_DEV0) && PtInitChannel (NRFCMD_DEV1)))
		return 0;

	nrf_powerlevel_last = nrf_powerlevel_current = -1;

	return 1;
}

/**********************************************************************/
void
led_set_rx (u_int8_t device, bool_t on)
{
	if (on)
		AT91F_PIO_ClearOutput (LED_BEACON_PIO,
							   device ? OB1_LED_BEACON_GREEN :
							   OB0_LED_BEACON_GREEN);
	else
		AT91F_PIO_SetOutput (LED_BEACON_PIO,
							 device ? OB1_LED_BEACON_GREEN :
							 OB0_LED_BEACON_GREEN);
}

/**********************************************************************/
static void
led_set_tx (u_int8_t device, bool_t on)
{
	if (on)
		AT91F_PIO_ClearOutput (LED_BEACON_PIO,
							   device ? OB1_LED_BEACON_RED :
							   OB0_LED_BEACON_RED);
	else
		AT91F_PIO_SetOutput (LED_BEACON_PIO,
							 device ? OB1_LED_BEACON_RED :
							 OB0_LED_BEACON_RED);
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
	return (src >> 24) | (src << 24) | ((src >> 8) & 0x0000FF00) | ((src << 8)
																	&
																	0x00FF0000);
}

/**********************************************************************/
static unsigned char
vnRF_ProcessDevice (u_int8_t device)
{
	u_int8_t status, duplicate;
	u_int16_t crc, oid, prox;
	u_int32_t unique, t, seconds_since_boot, time;
	static unsigned int reader_sequence = 0;

	/* handle RX off-blink */
	time = xTaskGetTickCount () / portTICK_RATE_MS;
	if (rf_lastblink[device] && (rf_lastblink[device] <= time))
	{
		rf_lastblink[device] = 0;
		led_set_rx (device, 0);
	}

	status = nRFAPI_GetFifoStatus (device);

	/* living in a paranoid world ;-) */
	if (status & FIFO_TX_FULL)
		nRFAPI_FlushTX (device);

	/* check for received packets */
	if (status & FIFO_RX_EMPTY)
		return 0;

	/* Setup log file header */
	g_Beacon.hdr.protocol = BEACONLOG_SIGHTING;
	g_Beacon.hdr.interface = device;
	g_Beacon.hdr.reader_id = swapshort ((u_int16_t) env.e.reader_id);
	g_Beacon.hdr.size = swapshort (sizeof (g_Beacon));

	do
	{
		/* read packet from nRF chip */
		nRFCMD_RegReadBuf (device, RD_RX_PLOAD, g_Beacon.log.byte,
						   sizeof (g_Beacon.log));

		rf_rec[device]++;

		/* get latest time */
		time = xTaskGetTickCount () / portTICK_RATE_MS;

		/* blink for LED_ON_RX_BLINK_DURATION_MS milliseconds */
		if (!rf_lastblink[device])
		{
			led_set_rx (device, 1);
			rf_lastblink[device] = time + LED_ON_RX_BLINK_DURATION_MS;
		}

		/* calculate checkum to check for duplicates */
		unique = crc32 (g_Beacon.log.byte, sizeof (g_Beacon.log.byte));

		/* check for duplicates */
		duplicate = pdFALSE;

		if (!env.e.filter_duplicates)
			/* reset rf_duplicates if detection is disabled */
			rf_duplicates[device] = 0;
		else
		{
			for (t = 0; t < DUPLICATES_BACKLOG_SIZE; t++)
				if (duplicate_backlog[t] == unique)
				{
					duplicate = pdTRUE;
					rf_duplicates[device]++;
					break;
				}

			/* add non-duplicates to CRC backlog */
			if (!duplicate)
			{
				duplicate_backlog[duplicate_backlog_pos++] = unique;
				if (duplicate_backlog_pos >= DUPLICATES_BACKLOG_SIZE)
					duplicate_backlog_pos = 0;
			}
		}

		if (!duplicate)
		{
			/* add time stamp & sequence number */
			g_Beacon.sequence = swaplong (reader_sequence++);
			g_Beacon.timestamp = swaplong (time);
			/* post packet to log file queue with CRC */
			crc = env_icrc16 ((u_int8_t *) & g_Beacon.hdr.protocol,
							  sizeof (g_Beacon) -
							  sizeof (g_Beacon.hdr.icrc16));
			g_Beacon.hdr.icrc16 = swapshort (crc);
			xQueueSend (xLogfile, &g_Beacon, 0);

			/* post packet to network via UDP */
			vNetworkSendBeaconToServer ();
		}

		if (pt_debug_level)
		{
			/* adjust byte order and decode */
			shuffle_tx_byteorder ();
			xxtea_decode ();
			shuffle_tx_byteorder ();

			rf_decrypt[device]++;

			/* verify the crc checksum */
			crc = env_crc16 (g_Beacon.log.byte,
							 sizeof (g_Beacon.log) - sizeof (u_int16_t));

			seconds_since_boot = time / 1000;

			if (swapshort (g_Beacon.log.pkt.crc) != crc)
			{
				rf_crc_err[device]++;
				debug_printf ("@%07u[%u] CRC error\n", seconds_since_boot,
							  device);
			}
			else
			{
				rf_crc_ok[device]++;

				oid = swapshort (g_Beacon.log.pkt.oid);
				if (g_Beacon.log.pkt.flags & RFBFLAGS_SENSOR)
					debug_printf ("@%07u[%u] BUTTON: %u\n",
								  seconds_since_boot, device, oid);

				switch (g_Beacon.log.pkt.proto)
				{
					case RFBPROTO_BEACONTRACKER:
						debug_printf ("@%07u[%u] RX: %u,0x%08X,%u,0x%02X\n",
									  seconds_since_boot,
									  device,
									  swapshort (g_Beacon.log.pkt.oid),
									  swaplong (g_Beacon.log.pkt.p.tracker.
												seq),
									  g_Beacon.log.pkt.p.tracker.strength,
									  g_Beacon.log.pkt.flags);
						break;

					case RFBPROTO_PROXREPORT:
						for (t = 0; t < PROX_MAX; t++)
						{
							prox =
								(swapshort
								 (g_Beacon.log.pkt.p.prox.oid_prox[t]));
							if (prox)
								debug_printf ("@%07u[%u] PX: %u={%u,%u,%u}\n",
											  seconds_since_boot,
											  device,
											  oid,
											  (prox >> 0) & 0x7FF,
											  (prox >> 14) & 0x3,
											  (prox >> 11) & 0x7);
							else
								debug_printf
									("@%07u[%u] RX: %u,          ,3,0x%02X\n",
									 seconds_since_boot,
									 device,
									 swapshort (g_Beacon.log.pkt.oid),
									 g_Beacon.log.pkt.flags);
						}
						break;

					default:
						debug_printf ("@%07u[%u] Uknown Protocol: %u\n",
									  seconds_since_boot,
									  device, g_Beacon.log.pkt.proto);
				}
			}
		}
	}
	while ((nRFAPI_GetFifoStatus (device) & FIFO_RX_EMPTY) == 0);

	/* did I already mention this paranoid world thing? */
	nRFAPI_ClearIRQ (device, MASK_IRQ_FLAGS);

	return 1;
}


/**********************************************************************/
static void
vnRFtaskRxTx (void *parameter)
{
	unsigned int delta_t_ms;
	portTickType time, time_old;

	if (!PtInitNRF ())
		while (1)
		{
			vTaskDelay (1000 / portTICK_RATE_MS);
			led_set_tx (NRFCMD_DEV0, 1);
			led_set_rx (NRFCMD_DEV0, 0);
			led_set_tx (NRFCMD_DEV1, 1);
			led_set_rx (NRFCMD_DEV1, 0);

			vTaskDelay (1000 / portTICK_RATE_MS);
			led_set_tx (NRFCMD_DEV0, 0);
			led_set_rx (NRFCMD_DEV0, 1);
			led_set_tx (NRFCMD_DEV1, 0);
			led_set_rx (NRFCMD_DEV1, 1);
		}
	led_set_tx (NRFCMD_DEV0, 1);
	led_set_tx (NRFCMD_DEV1, 1);

	time_old = xTaskGetTickCount () * portTICK_RATE_MS;

	vLedSetGreen (1);

	for (;;)
	{
		/* gather statistics */
		time = xTaskGetTickCount () * portTICK_RATE_MS;
		delta_t_ms = time - time_old;

		if (delta_t_ms > 1000)
		{
			time_old = time;

			rf_pkt_per_sec[0] =
				(rf_rec[0] - rf_rec_old[0]) * 1000 / delta_t_ms;
			rf_rec_old[0] = rf_rec[0];

			rf_pkt_per_sec[1] =
				(rf_rec[1] - rf_rec_old[1]) * 1000 / delta_t_ms;
			rf_rec_old[1] = rf_rec[1];
		}

		/* check if TX strength changed */
		if (nrf_powerlevel_current != nrf_powerlevel_last)
		{
			nRFAPI_SetTxPower (NRFCMD_DEV0, nrf_powerlevel_current);
			nRFAPI_SetTxPower (NRFCMD_DEV1, nrf_powerlevel_current);
			nrf_powerlevel_last = nrf_powerlevel_current;
		}

#ifndef DISABLE_WATCHDOG
		/* Restart watchdog, has been enabled in Cstartup_SAM7.c */
		AT91F_WDTRestart (AT91C_BASE_WDTC);
#endif /*DISABLE_WATCHDOG */

		if (vnRF_ProcessDevice (NRFCMD_DEV0))
		{
			vnRF_ProcessDevice (NRFCMD_DEV1);
			vTaskDelay (0);
		}
		else if (vnRF_ProcessDevice (NRFCMD_DEV1))
			vTaskDelay (0);
		else
			nRFCMD_WaitRx (200);
	}
}

/**********************************************************************/
void
PtStatusRxTxDevice (u_int8_t device)
{
	if (device > NRFCMD_DEV_MAX)
		return;

	debug_printf ("\nOpenBeacon Interface[%u] Status:\n", device);
	debug_printf ("\treceived    = %u\n", rf_rec[device]);
	debug_printf ("\treceive rate= %u packets/second\n",
				  rf_pkt_per_sec[device]);
	if (env.e.filter_duplicates)
		debug_printf ("\tduplicates  = %u\n", rf_duplicates[device]);

	if (pt_debug_level)
	{
		debug_printf ("\tdecrypted   = %u\n", rf_decrypt[device]);
		debug_printf ("\tpkt crc ok  = %u\n", rf_crc_ok[device]);
		debug_printf ("\tpkt crc err = %u\n", rf_crc_err[device]);
	}
	debug_printf ("\n");
}

/**********************************************************************/
void
PtStatusRxTx (void)
{
	u_int8_t device;

	for (device = 0; device <= NRFCMD_DEV_MAX; device++)
		PtStatusRxTxDevice (device);
}

/**********************************************************************/
static void
vnRFLogFileFileTask (void *parameter)
{
	UINT written;
	FRESULT res;
	static TBeaconLogSighting data;
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
					debug_printf
						("OpenBeacon firmware version %s\nreader_id=%i.\n",
						 VERSION, env.e.reader_id);

					/* Storing clock ticks for flushing cache action */
					time_old = xTaskGetTickCount ();

					for (;;)
					{
						/* query queue for new data with a 500ms timeout */
						if (xQueueReceive
							(xLogfile, &data, 500 * portTICK_RATE_MS))
						{
							if (f_write
								(&fil, &data, sizeof (data), &written)
								|| written != sizeof (data))
								debug_printf
									("\nfailed to write to logfile\n");
						}

						/* flush file every 10 seconds */
						time = xTaskGetTickCount ();
						if ((time - time_old) >= (10000 * portTICK_RATE_MS))
						{
							time_old = time;
							if (f_sync (&fil))
								debug_printf
									("\nfailed to flush to logfile\n");
						}
					}
				}
				break;

			default:
				debug_printf ("Error (%i) while searching for logfile '%s'\n",
							  res, logfile);
		}

	/* error blinking - twice per second with 0.8s spacing */
	while (1)
	{
		led_set_red (1);
		vTaskDelay (25 / portTICK_RATE_MS);
		led_set_red (0);
		vTaskDelay (150 / portTICK_RATE_MS);
		led_set_red (1);
		vTaskDelay (25 / portTICK_RATE_MS);
		led_set_red (0);
		vTaskDelay (800 / portTICK_RATE_MS);
	}
}

/**********************************************************************/
void
PtInitProtocol (void)
{
	/* turn off LEDs */
	AT91F_PIO_CfgOutput (LED_BEACON_PIO, LED_BEACON_MASK);
	AT91F_PIO_SetOutput (LED_BEACON_PIO, LED_BEACON_MASK);

	rf_rec[0] = rf_rec_old[0] = rf_decrypt[0] = 0;
	rf_crc_ok[0] = rf_crc_err[0] = rf_pkt_per_sec[0] = 0;
	rf_rec[1] = rf_rec_old[1] = rf_decrypt[1] = 0;
	rf_crc_ok[1] = rf_crc_err[1] = rf_pkt_per_sec[1] = 0;
	rf_lastblink[0] = rf_lastblink[1] = 0;

	duplicate_backlog_pos = rf_duplicates[0] = rf_duplicates[1] = 0;
	memset (&duplicate_backlog, 0, sizeof (duplicate_backlog));

	/* xLogfile queue can hold now 4kB of data */
	xLogfile =
		xQueueCreate ((4 * 1024) / sizeof (g_Beacon), sizeof (g_Beacon));

	xTaskCreate (vnRFtaskRxTx, (signed portCHAR *) "nRF_RxTx", TASK_NRF_STACK,
				 NULL, TASK_NRF_PRIORITY, NULL);

	xTaskCreate (vnRFLogFileFileTask, (signed portCHAR *) "nRF_Logfile",
				 TASK_FILE_STACK, NULL, TASK_FILE_PRIORITY, NULL);
}
