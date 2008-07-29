/***************************************************************
 *
 * OpenBeacon.org - OpenBeacon dimmer link layer protocol
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
#include "proto.h"
#include "nRF24L01/nRF_HW.h"
#include "nRF24L01/nRF_CMD.h"
#include "nRF24L01/nRF_API.h"

const unsigned char broadcast_mac[NRF_MAX_MAC_SIZE] = { 1, 2, 3, 2, 1 };
TBeaconEnvelope g_Beacon;

#define PWM_CMR_PRESCALER 0x3
#define PWM_CMR_CLOCK_FREQUENCY (MCK/(1<<PWM_CMR_PRESCALER))

// set dimmer default brightness to 50%
#define PWM_CMR_DEFAULT_DIMMER  (PWM_CMR_CLOCK_FREQUENCY/200)

// set LED flash time for triac trigger to 250us
#define PWM_CMR_DIMMER_LED_TIME (PWM_CMR_CLOCK_FREQUENCY/400)

#define BLINK_INTERVAL_MS (50 / portTICK_RATE_MS)

/**********************************************************************/
#define SHUFFLE(a,b)    tmp=g_Beacon.datab[a];\
                        g_Beacon.datab[a]=g_Beacon.datab[b];\
                        g_Beacon.datab[b]=tmp;

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

void
vnRFtaskRx (void *parameter)
{
  u_int16_t crc;
  (void) parameter;
  int DidBlink=0;
  portTickType Ticks=0;
  
  if (!PtInitNRF ())
    return;

  for (;;)
    {
      if (nRFCMD_WaitRx (10))
	{
	  if(!DidBlink)
	  {
	    vLedSetGreen (1);
	    Ticks = xTaskGetTickCount();
	    DidBlink = 1;
	  }

	  do
	    {
	      // read packet from nRF chip
	      nRFCMD_RegReadBuf (RD_RX_PLOAD, g_Beacon.datab,
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
		{
		  DumpStringToUSB ("RX: ");
		  DumpUIntToUSB (swaplong (g_Beacon.pkt.oid));
		  DumpStringToUSB (",");
		  DumpUIntToUSB (swaplong (g_Beacon.pkt.seq));
		  DumpStringToUSB (",");
		  DumpUIntToUSB (g_Beacon.pkt.strength);
		  DumpStringToUSB (",");
		  DumpUIntToUSB (g_Beacon.pkt.flags);
		  DumpStringToUSB ("\n\r");
		}
	    }
	  while ((nRFAPI_GetFifoStatus () & FIFO_RX_EMPTY) == 0);
	}
      nRFAPI_ClearIRQ (MASK_IRQ_FLAGS);
    
      if( DidBlink && ((xTaskGetTickCount()-Ticks)>BLINK_INTERVAL_MS) )
      {
        DidBlink=0;
	vLedSetGreen(0);
      }
    }
}

static inline void
vUpdateDimmer (int Percent)
{
    int t;
    
    if(Percent<1)
      Percent=1;
    else
	if(Percent>90)
	    Percent=90;
	    
    t = ((PWM_CMR_CLOCK_FREQUENCY*Percent)/(100*120));
    AT91C_BASE_TC2->TC_RA = t;
    AT91C_BASE_TC2->TC_RC = t+PWM_CMR_DIMMER_LED_TIME;
}

static inline void
vInitDimmer (void)
{
    /* Enable Peripherals */
    AT91F_PIO_CfgPeriph(AT91C_BASE_PIOA, 0, TRIGGER_PIN|PHASE_PIN);

    /* Configure Timer/Counter */    
    AT91F_TC2_CfgPMC();
    AT91C_BASE_TC2->TC_IDR = 0xFF;
    AT91C_BASE_TC2->TC_CCR = AT91C_TC_CLKDIS;
    AT91C_BASE_TC2->TC_CMR =
	AT91C_TC_CLKS_TIMER_DIV2_CLOCK |
	AT91C_TC_CPCSTOP |
	AT91C_TC_EEVTEDG_RISING |
	AT91C_TC_EEVT_TIOB |
	AT91C_TC_ENETRG |
	AT91C_TC_WAVE |
	AT91C_TC_WAVESEL_UP_AUTO |
	AT91C_TC_ACPA_SET |
	AT91C_TC_ACPC_CLEAR;
    AT91C_BASE_TC2->TC_RA = PWM_CMR_DEFAULT_DIMMER;
    AT91C_BASE_TC2->TC_RC = PWM_CMR_DEFAULT_DIMMER+PWM_CMR_DIMMER_LED_TIME;
    AT91C_BASE_TC2->TC_CCR = AT91C_TC_CLKEN;

    AT91C_BASE_TCB->TCB_BCR=AT91C_TCB_SYNC;
    AT91C_BASE_TCB->TCB_BMR=AT91C_TCB_TC0XC0S_NONE|AT91C_TCB_TC1XC1S_NONE|AT91C_TCB_TC2XC2S_NONE;
}

void
vnRFtaskDimmer (void *parameter)
{
    (void) parameter;
    int Percent=0,Sign=1;

    (void) parameter;
    
    while(1)
    {
	vTaskDelay(20 / portTICK_RATE_MS);	
	
	Percent+=Sign;
	if(Percent>=70)
	    Sign=-1;
	else
	    if(Percent<=20)
		Sign=1;
	
	vUpdateDimmer(Percent);
    }
}

void
vInitProtocolLayer (void)
{
    vInitDimmer ();
    xTaskCreate (vnRFtaskRx, (signed portCHAR *) "nRF_Rx", TASK_NRF_STACK,
	NULL, TASK_NRF_PRIORITY, NULL);
    
    xTaskCreate (vnRFtaskDimmer, (signed portCHAR *) "Dimmer", TASK_NRF_STACK,
	NULL, TASK_NRF_PRIORITY, NULL);
}
