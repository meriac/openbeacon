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
#include "debug_printf.h"
#include "proto.h"
#include "nRF24L01/nRF_HW.h"
#include "nRF24L01/nRF_CMD.h"
#include "nRF24L01/nRF_API.h"

const unsigned char broadcast_mac[NRF_MAX_MAC_SIZE] = { 1, 2, 3, 2, 1 };

TBeaconEnvelope g_Beacon;

/**********************************************************************/
#define SHUFFLE(a,b)    tmp=g_Beacon.datab[a];\
                        g_Beacon.datab[a]=g_Beacon.datab[b];\
                        g_Beacon.datab[b]=tmp;

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
		(DEFAULT_DEV, DEFAULT_CHANNEL, broadcast_mac, sizeof (broadcast_mac),
		 ENABLED_NRF_FEATURES))
		return 0;

	nRFAPI_SetPipeSizeRX (DEFAULT_DEV, 0, 16);
	nRFAPI_SetTxPower (DEFAULT_DEV, 3);
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
	return (src >> 24) | (src << 24) | ((src >> 8) & 0x0000FF00) | ((src << 8)
																	&
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
	u_int16_t crc;
	(void) parameter;

	if (!PtInitNRF ())
		return;

	vLedSetGreen (1);

	for (;;)
	{
		if (nRFCMD_WaitRx (10))
		{
			vLedSetRed (1);

			do
			{
				// read packet from nRF chip
				nRFCMD_RegReadBuf (DEFAULT_DEV, RD_RX_PLOAD, g_Beacon.datab,
								   sizeof (g_Beacon));

				// adjust byte order and decode
				shuffle_tx_byteorder ();
				xxtea_decode ();
				shuffle_tx_byteorder ();

				// verify the crc checksum
				crc =
					crc16 (g_Beacon.datab,
						   sizeof (g_Beacon) - sizeof (g_Beacon.pkt.crc));
				if ((swapshort (g_Beacon.pkt.crc) == crc))
					hex_dump ((u_int8_t *) & g_Beacon, 0, sizeof (g_Beacon));
				else
					debug_printf ("RX: CRC error or wrong encryption key\n");
			}
			while ((nRFAPI_GetFifoStatus (DEFAULT_DEV) & FIFO_RX_EMPTY) == 0);

			vLedSetRed (0);
			nRFAPI_GetFifoStatus (DEFAULT_DEV);
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
