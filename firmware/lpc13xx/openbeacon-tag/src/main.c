/***************************************************************
 *
 * OpenBeacon.org - main file for OpenBeacon Sensor (CR123A)
 *
 * Copyright 2010 Milosch Meriac <meriac@openbeacon.de>
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
#include <openbeacon.h>
#include "3d_acceleration.h"
#include "sound.h"
#include "xxtea.h"
#include "pmu.h"
#include "iap.h"
#include "spi.h"
#include "nRF_API.h"
#include "nRF_CMD.h"
#include "usbserial.h"
#include "openbeacon-proto.h"

uint32_t g_sysahbclkctrl;

#define FIFO_DEPTH 10
typedef struct
{
  int x, y, z;
} TFifoEntry;

#define MAINCLKSEL_IRC 0
#define MAINCLKSEL_SYSPLL_IN 1
#define MAINCLKSEL_WDT 2
#define MAINCLKSEL_SYSPLL_OUT 3

#define TONE_HIGH 23

/* device UUID */
static uint16_t tag_id;
static TDeviceUID device_uuid;
static uint32_t random_seed;

/* OpenBeacon packet */
/* Default TEA encryption key of the tag - MUST CHANGE ! */
static const uint32_t xxtea_key[4] = { 0x00112233, 0x44556677, 0x8899AABB, 0xCCDDEEFF };

/* set nRF24L01 broadcast mac */
static const unsigned char broadcast_mac[NRF_MAX_MAC_SIZE] = { 1, 2, 3, 3, 1 };

uint32_t
random (uint32_t range)
{
  static uint32_t v1 = 0x52f7d319;
  static uint32_t v2 = 0x6e28014a;

  /* MWC generator, period length 1014595583
     according to George Marsaglia */
  return ((((v1 = 36969 * (v1 & 0xffff) + (v1 >> 16)) << 16) ^
	   (v2 = 30963 * (v2 & 0xffff) + (v2 >> 16))) ^ random_seed) % range;
}

int
main (void)
{
  volatile int i;
  int x, y, z;
  /* wait on boot - debounce */
  for (i = 0; i < 2000000; i++);

  /* Initialize GPIO (sets up clock) */
  GPIOInit ();

  /* initialize power management */
  pmu_init ();

  LPC_IOCON->PIO2_0 = 0;
  GPIOSetDir (2, 0, 1);		//OUT
  GPIOSetValue (2, 0, 0);

  LPC_IOCON->RESET_PIO0_0 = 0;
  GPIOSetDir (0, 0, 0);		//IN

  LPC_IOCON->PIO0_1 = 0;
  GPIOSetDir (0, 1, 0);		//IN

  LPC_IOCON->PIO1_8 = 0;
  GPIOSetDir (1, 8, 1);		//OUT
  GPIOSetValue (1, 8, 0);

  LPC_IOCON->PIO0_2 = 0;
  GPIOSetDir (0, 2, 1);		//OUT
  GPIOSetValue (0, 2, 0);

  LPC_IOCON->PIO0_3 = 0;
  GPIOSetDir (0, 3, 0);		//IN

  LPC_IOCON->PIO0_4 = 1 << 8;
  GPIOSetDir (0, 4, 1);		//OUT
  GPIOSetValue (0, 4, 1);

  LPC_IOCON->PIO0_5 = 1 << 8;
  GPIOSetDir (0, 5, 1);		//OUT
  GPIOSetValue (0, 5, 1);

  LPC_IOCON->PIO1_9 = 0;	//FIXME
  GPIOSetDir (1, 9, 1);		//OUT
  GPIOSetValue (1, 9, 0);

  LPC_IOCON->PIO0_6 = 0;
  GPIOSetDir (0, 6, 1);		//OUT
  GPIOSetValue (0, 6, 1);

  LPC_IOCON->PIO0_7 = 0;
  GPIOSetDir (0, 7, 1);		//OUT
  GPIOSetValue (0, 7, 0);

  /* select UART_TXD */
  LPC_IOCON->PIO1_7 = 1;

  LPC_IOCON->PIO1_6 = 0;
  GPIOSetDir (1, 6, 1);		//OUT
  GPIOSetValue (1, 6, 0);

  LPC_IOCON->PIO1_5 = 0;
  GPIOSetDir (1, 5, 1);		//OUT
  GPIOSetValue (1, 5, 0);

  LPC_IOCON->PIO3_2 = 0;	// FIXME
  GPIOSetDir (3, 2, 1);		//OUT
  GPIOSetValue (3, 2, 1);

  LPC_IOCON->PIO1_11 = 0x80;	//FIXME
  GPIOSetDir (1, 11, 1);	// OUT
  GPIOSetValue (1, 11, 0);

  LPC_IOCON->PIO1_4 = 0x80;
  GPIOSetDir (1, 4, 0);		// IN

  LPC_IOCON->ARM_SWDIO_PIO1_3 = 0x81;
  GPIOSetDir (1, 3, 1);		// OUT
  GPIOSetValue (1, 3, 0);

  LPC_IOCON->JTAG_nTRST_PIO1_2 = 0x81;
  GPIOSetDir (1, 2, 1);		// OUT
  GPIOSetValue (1, 2, 0);

  LPC_IOCON->JTAG_TDO_PIO1_1 = 0x81;
  GPIOSetDir (1, 1, 1);		// OUT
  GPIOSetValue (1, 1, 0);

  LPC_IOCON->JTAG_TMS_PIO1_0 = 0x81;
  GPIOSetDir (1, 0, 1);		// OUT
  GPIOSetValue (1, 0, 0);

  LPC_IOCON->JTAG_TDI_PIO0_11 = 0x81;
  GPIOSetDir (0, 11, 1);	// OUT
  GPIOSetValue (0, 11, 0);

  LPC_IOCON->PIO1_10 = 0x80;
  GPIOSetDir (1, 10, 1);	// OUT
  GPIOSetValue (1, 10, 1);

  LPC_IOCON->JTAG_TCK_PIO0_10 = 0x81;
  GPIOSetDir (0, 10, 1);	// OUT
  GPIOSetValue (0, 10, 0);

  LPC_IOCON->PIO0_9 = 0;
  GPIOSetDir (0, 9, 1);		// OUT
  GPIOSetValue (0, 9, 0);

  /* select MISO function for PIO0_8 */
  LPC_IOCON->PIO0_8 = 1;

  /* NVIC is installed inside UARTInit file. */
  UARTInit (115200, 0);

  /* initialize USB serial interface */
  init_usbserial ();

  /* initialize SPI */
  spi_init ();

#ifdef	SOUND_ENABLE
  /* Init Speaker Output */
  snd_init ();
#endif /*SOUND_ENABLE */

  /* Init 3D acceleration sensor */
  acc_init (0);

  /* read device UUID */
  bzero (&device_uuid, sizeof (device_uuid));
  iap_read_uid (&device_uuid);
  tag_id = crc16 ((uint8_t *) & device_uuid, sizeof (device_uuid));
  random_seed =
    device_uuid[0] ^
    device_uuid[1] ^
    device_uuid[2] ^
    device_uuid[3];

  /* Initialize OpenBeacon nRF24L01 interface */
  if (!nRFAPI_Init (81, broadcast_mac, sizeof (broadcast_mac), 0))
    for (;;)
      {
	GPIOSetValue (1, 3, 1);
	pmu_wait_ms (50);
	GPIOSetValue (1, 3, 0);
	pmu_wait_ms (950);
      }
  /* set retries to zero */
  nRFAPI_TxRetries (1);
  /* enable ACK */
  nRFAPI_SetPipeSizeRX (0, NRF_MAX_MAC_SIZE);
  nRFAPI_PipesAck (ERX_P0);
  nRFAPI_PipesEnable (ERX_P0);
  /* set tx power power to high */
  nRFAPI_SetTxPower (1);
  nRFCMD_Power (1);

  /* blink LED for 1s to show readyness */
  snd_tone (23);
  GPIOSetValue (1, 3, 1);
  pmu_wait_ms (1000);
  GPIOSetValue (1, 3, 0);
  snd_tone (0);

  i=0;
  while (1)
    {
	/* read acceleration sensor */
	nRFAPI_SetRxMode (0);
	acc_power (1);
	pmu_wait_ms (20);
	acc_xyz_read (&x, &y, &z);
	acc_power (0);

	GPIOSetValue (1, 3, 1);
	pmu_wait_ms (100);
	GPIOSetValue (1, 3, 0);
	pmu_wait_ms (100);
	GPIOSetValue (1, 3, 1);
	pmu_wait_ms (100);
	GPIOSetValue (1, 3, 0);
	pmu_wait_ms (1000);
	debug_printf ("Hello World %08u [%04i,%04i,%04i]\n",i++,x,y,z);
    }

  return 0;
}
