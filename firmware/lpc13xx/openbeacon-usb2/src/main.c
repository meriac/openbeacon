/***************************************************************
 *
 * OpenBeacon.org - main file for OpenBeacon USB II Bluetooth
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
#include "storage.h"
#include "bluetooth.h"
#include "pmu.h"
#include "iap.h"
#include "spi.h"
#include "nRF_API.h"
#include "nRF_CMD.h"
#include "xxtea.h"
#include "openbeacon-proto.h"

/* device UUID */
static uint16_t tag_id;
static TDeviceUID device_uuid;
/* random seed */
static uint32_t random_seed;

#define TX_STRENGTH_OFFSET 2

#define MAINCLKSEL_IRC 0
#define MAINCLKSEL_SYSPLL_IN 1
#define MAINCLKSEL_WDT 2
#define MAINCLKSEL_SYSPLL_OUT 3


/* Default TEA encryption key of the tag - MUST CHANGE ! */
static const uint32_t xxtea_key[4] = {
  0x00112233,
  0x44556677,
  0x8899AABB,
  0xCCDDEEFF
};

/* set nRF24L01 broadcast mac */
static const unsigned char broadcast_mac[NRF_MAX_MAC_SIZE] = {
  1, 2, 3, 2, 1
};

/* OpenBeacon packet */
static TBeaconEnvelope g_Beacon;

static void
nRF_tx (uint8_t power)
{

  /* encrypt data */
  xxtea_encode (g_Beacon.block, XXTEA_BLOCK_COUNT, xxtea_key);

  /* set TX power */
  nRFAPI_SetTxPower (power & 0x3);

  /* upload data to nRF24L01 */
  nRFAPI_TX ((uint8_t *) & g_Beacon, sizeof (g_Beacon));

  /* transmit data */
  nRFCMD_CE (1);

  /* wait for packet to be transmitted */
  pmu_sleep_ms (2);

  /* transmit data */
  nRFCMD_CE (0);
}

void
nrf_off (void)
{
  /* disable RX mode */
  nRFCMD_CE (0);

  /* wait till RX is done */
  pmu_sleep_ms (5);

  /* switch to TX mode */
  nRFAPI_SetRxMode (0);
}

static uint32_t
rnd (uint32_t range)
{
  static uint32_t v1 = 0x52f7d319;
  static uint32_t v2 = 0x6e28014a;

  /* reseed random with timer */
  random_seed += LPC_TMR32B0->TC;

  /* MWC generator, period length 1014595583 */
  return ((((v1 = 36969 * (v1 & 0xffff) + (v1 >> 16)) << 16) ^
	   (v2 = 30963 * (v2 & 0xffff) + (v2 >> 16))) ^ random_seed) % range;
}

static inline void
pin_init (void)
{
  LPC_IOCON->PIO2_0 = 0;
  GPIOSetDir (2, 0, 1);		//OUT
  GPIOSetValue (2, 0, 0);

  LPC_IOCON->RESET_PIO0_0 = 0;
  GPIOSetDir (0, 0, 0);		//IN

  LPC_IOCON->PIO0_1 = 0;
  GPIOSetDir (0, 1, 0);		//IN

  LPC_IOCON->PIO1_8 = 0;
  GPIOSetDir (1, 8, 1);		//OUT
  GPIOSetValue (1, 8, 1);

  LPC_IOCON->PIO0_2 = 0;
  GPIOSetDir (0, 2, 1);		//OUT
  GPIOSetValue (0, 2, 0);

  LPC_IOCON->PIO0_3 = 0;
  GPIOSetDir (0, 3, 0);		//IN

  LPC_IOCON->PIO0_4 = 1 << 8;
  GPIOSetDir (0, 4, 1);		//OUT
  GPIOSetValue (0, 4, 1);

  /* switch PMU to high power mode */
  LPC_IOCON->PIO0_5 = 1 << 8;
  GPIOSetDir (0, 5, 1); //OUT
  GPIOSetValue (0, 5, 0);

  LPC_IOCON->PIO1_9 = 0;	//FIXME
  GPIOSetDir (1, 9, 1);		//OUT
  GPIOSetValue (1, 9, 0);

  LPC_IOCON->PIO0_6 = 0;
  GPIOSetDir (0, 6, 1);		//OUT
  GPIOSetValue (0, 6, 1);

  LPC_IOCON->PIO0_7 = 0;
  GPIOSetDir (0, 7, 1);		//OUT
  GPIOSetValue (0, 7, 0);

  LPC_IOCON->PIO1_7 = 0;
  GPIOSetDir (1, 7, 1);		//OUT
  GPIOSetValue (1, 7, 0);

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
}

static inline void
show_version (void)
{
  debug_printf (" * Device UID: %08X:%08X:%08X:%08X\n",
    device_uuid[0], device_uuid[1],
    device_uuid[2], device_uuid[3]);
  debug_printf (" * OpenBeacon MAC: %02X:%02X:%02X:%02X:%02X\n",
    broadcast_mac[0], broadcast_mac[1], broadcast_mac[2],
    broadcast_mac[3], broadcast_mac[4]);
  debug_printf (" *         Tag ID: %04X\n", tag_id);
}

static inline void
main_menue (uint8_t cmd)
{
  /* ignore non-printable characters */
  if (cmd <= ' ')
    return;
  /* show key pressed */
  debug_printf ("%c\n", cmd);
  /* map lower case to upper case */
  if (cmd > 'a' && cmd < 'z')
    cmd -= ('a' - 'A');

  switch (cmd)
    {
    case '?':
    case 'H':
      debug_printf ("\n"
		    " *****************************************************\n"
		    " * OpenBeacon Tag - Bluetooth Console                *\n"
		    " * (C) 2011 Milosch Meriac <meriac@openbeacon.de>    *\n"
		    " *****************************************************\n"
		    " * H,?          - this help screen\n"
		    " * S            - Show device status\n"
		    " * E            - Reset Logfile\n"
		    " *****************************************************\n"
		    "\n");
      break;
    case 'S':
      debug_printf ("\n"
		    " *****************************************************\n"
		    " * OpenBeacon Status Information                     *\n"
		    " *****************************************************\n");
      show_version ();
      spi_status ();
      acc_status ();
      storage_status ();
      debug_printf (" *****************************************************\n"
		    "\n");
      break;

    case 'E':
      debug_printf ("\nErasing Storage...\n\n");
      storage_erase ();
      break;

    default:
      debug_printf ("Unknown command '%c' - please press 'H' for help \n",
		    cmd);
    }
  debug_printf ("\n# ");
}

int
main (void)
{
  uint32_t SSPdiv;
  uint16_t crc, oid_last_seen;
  uint8_t status, seen_low, seen_high;
  volatile int t;
  int i;

  /* wait on boot - debounce */
  for (t = 0; t < 2000000; t++);

  /* Initialize GPIO (sets up clock) */
  GPIOInit ();

  /* initialize pins */
  pin_init ();

  /* fire up LED 1 */
  GPIOSetValue (1, 1, 1);

  /* initialize SPI */
  spi_init ();

  /* read device UUID */
  bzero (&device_uuid, sizeof (device_uuid));
  iap_read_uid (&device_uuid);
  tag_id = crc16 ((uint8_t *) & device_uuid, sizeof (device_uuid));
  random_seed =
    device_uuid[0] ^ device_uuid[1] ^ device_uuid[2] ^ device_uuid[3];

  /* Plugged to computer upon reset ? */
  if(GPIOGetValue (0,3))
  {
    /* wait some time till Bluetooth is off */
    for (t = 0; t < 2000000; t++);

    /* Init 3D acceleration sensor */
    acc_init (1);
    /* Init Flash Storage */
    storage_init (tag_id);
    /* Init Bluetooth */
    bt_init (1, tag_id);

    /* switch to LED 2 */
    GPIOSetValue (1, 1, 0);
    GPIOSetValue (1, 2, 1);

    /* spin in loop */
    while(1)
    {
      /* reset after USB unplug */
      if(!GPIOGetValue (0,3))
	NVIC_SystemReset ();

      /* if UART rx send to menue */
      if(UARTCount)
      {
	/* blink LED1 upon Bluetooth command */
	GPIOSetValue (1, 1, 1);
	/* execute menue command with last character received */
	main_menue(UARTBuffer[UARTCount - 1]);
	/* reset UART buffer */
	UARTCount = 0;
	/* un-blink LED1 */
	GPIOSetValue (1, 1, 0);
      }
    }
  }

  /* initialize power management */
  pmu_init ();

  /* Init 3D acceleration sensor */
  acc_init (0);

  /* Initialize OpenBeacon nRF24L01 interface */
  if (!nRFAPI_Init (81, broadcast_mac, sizeof (broadcast_mac), 0))
  for (;;)
  {
    GPIOSetValue (1, 2, 1);
    pmu_sleep_ms (500);
    GPIOSetValue (1, 2, 0);
    pmu_sleep_ms (500);
  }
  /* set tx power power to high */
  nRFCMD_Power (1);

  /* blink LED for 1s to show readyness */
  GPIOSetValue (1, 1, 0);
  GPIOSetValue (1, 2, 1);
  pmu_sleep_ms (1000);
  GPIOSetValue (1, 2, 0);

  /* disable unused jobs */
  SSPdiv = LPC_SYSCON->SSPCLKDIV;
  i = 0;
  seen_low = seen_high = 0;
  oid_last_seen = 0;
  while (1)
    {
      /* transmit every 50-150ms */
      pmu_sleep_ms (50 + rnd (100));

      /* getting SPI back up again */
      LPC_SYSCON->SSPCLKDIV = SSPdiv;

      /* blink every 16th packet transmitted */
      if ((i & 0xF) == 0)
	{
	  /* switch to RX mode */
	  nRFAPI_SetRxMode (1);
	  nRFCMD_CE (1);
	  pmu_sleep_ms (20);
	  nRFCMD_CE (0);
	  /* fire up LED */
	  GPIOSetValue (1, 2, 1);
	  /* wait till RX stops */
	  pmu_sleep_ms (10);
	  /* turn LED off */
	  GPIOSetValue (1, 2, 0);
	  nRFAPI_SetRxMode (0);
	  if (nRFCMD_IRQ ())
	    {
	      do
		{
		  // read packet from nRF chip
		  nRFCMD_RegReadBuf (RD_RX_PLOAD, g_Beacon.byte,
				     sizeof (g_Beacon));

		  // adjust byte order and decode
		  xxtea_decode (g_Beacon.block, XXTEA_BLOCK_COUNT, xxtea_key);

		  // verify the CRC checksum
		  crc = crc16 (g_Beacon.byte,
			       sizeof (g_Beacon) - sizeof (uint16_t));

		  if ((ntohs (g_Beacon.pkt.crc) == crc) &&
		      (g_Beacon.pkt.proto == RFBPROTO_BEACONTRACKER))
		    {
		      oid_last_seen = g_Beacon.pkt.oid;
		      switch (g_Beacon.pkt.p.tracker.strength)
			{
			case TX_STRENGTH_OFFSET:
			  seen_low++;
			  break;
			case TX_STRENGTH_OFFSET + 1:
			  seen_high++;
			  break;
			}
		      /* fire up LED to indicate rx */
		      GPIOSetValue (1, 1, 1);
		      /* light LED for 2ms */
		      pmu_sleep_ms (2);
		      /* turn LED off */
		      GPIOSetValue (1, 1, 0);
		    }
		  status = nRFAPI_GetFifoStatus ();
		}
	      while ((status & FIFO_RX_EMPTY) == 0);
	    }
	  nRFAPI_ClearIRQ (MASK_IRQ_FLAGS);
	}
      else
	{
	  /* powering up nRF24L01 */
	  nRFAPI_SetRxMode (0);

	  /* prepare packet */
	  bzero (&g_Beacon, sizeof (g_Beacon));
	  g_Beacon.pkt.proto = RFBPROTO_BEACONTRACKER;
	  g_Beacon.pkt.oid = htons (tag_id);
	  g_Beacon.pkt.p.tracker.strength = (i & 1) + TX_STRENGTH_OFFSET;
	  g_Beacon.pkt.p.tracker.seq = htonl (LPC_TMR32B0->TC);
	  g_Beacon.pkt.p.tracker.oid_last_seen = oid_last_seen;
	  g_Beacon.pkt.p.tracker.seen_low = seen_low;
	  g_Beacon.pkt.p.tracker.seen_high = seen_high;
	  g_Beacon.pkt.p.tracker.reserved = 0;
	  g_Beacon.pkt.crc =
	    htons (crc16
		   (g_Beacon.byte,
		    sizeof (g_Beacon) - sizeof (g_Beacon.pkt.crc)));

	  /* set tx power to low */
	  nRFCMD_Power (0);
	  /* transmit packet */
	  nRF_tx (g_Beacon.pkt.p.tracker.strength);
	  nRFCMD_Power (1);
	}

      /* powering down */
      nRFAPI_PowerDown ();
      LPC_SYSCON->SSPCLKDIV = 0x00;

      /* increment counter */
      i++;
    }
  return 0;
}
