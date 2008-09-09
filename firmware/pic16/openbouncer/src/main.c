/***************************************************************
 *
 * OpenBeacon.org - OpenBoucer cryptographic door lock system
 *                  main entry, CRC, behaviour
 *
 * Copyright 2006 Milosch Meriac <meriac@openbeacon.de>
 *
/***************************************************************

/*
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

#define OPENBEACON_ENABLEENCODE

#include <htc.h>
#include <stdlib.h>
#include "config.h"
#include "timer.h"
#include "openbouncer.h"
#include "protocol.h"
#include "nRF_CMD.h"
#include "nRF_HW.h"
#include "main.h"

__CONFIG (0x03d4);

TMacroBeacon g_MacroBeacon = {
  sizeof (TBouncerEnvelope) + 1,	// Size
  WR_TX_PLOAD			// Transmit Packet Opcode
};

/* switch to rx mode */
static void
switch_to_rxmode (void)
{
  nRFCMD_RegExec (FLUSH_RX);
  nRFCMD_ClearIRQ (MASK_IRQ_FLAGS);
  nRFCMD_DoRX (1);
  CONFIG_PIN_CE = 1;
}

/* switch to tx mode */
static void
switch_to_txmode (void)
{
  CONFIG_PIN_CE = 0;
  sleep_2ms ();
  nRFCMD_DoRX (0);
  nRFCMD_RegExec (FLUSH_RX);
  nRFCMD_ClearIRQ (MASK_IRQ_FLAGS);
}

/* send away g_MacroBeacon.cmd packet */
static void
packet_tx (void)
{
  nRFCMD_Macro ((unsigned char *) &g_MacroBeacon);
  nRFCMD_Execute ();
}

/* try to find specified command in RX fifo */
static unsigned char
packet_rx (unsigned char cmd)
{
  unsigned char res = 0;

  /* read every packet from FIFO */
  while ((nRFCMD_GetFifoStatus () & FIFO_RX_EMPTY) == 0)
    {
      /* read packet from nRF chip */
      nRFCMD_RegRead (RD_RX_PLOAD,
		      (unsigned char *) &g_MacroBeacon.cmd,
		      sizeof (g_MacroBeacon.cmd));

      /* search for BOUNCERPKT_CMD_CHALLENGE_SETUP */
      if ((g_MacroBeacon.cmd.hdr.version == BOUNCERPKT_VERSION) &&
	  (g_MacroBeacon.cmd.hdr.command == cmd))
	{
	  /* turn on LED */
	  CONFIG_PIN_LED = 1;

	  res = 1;
	  /* clear RX fifo */
	  nRFCMD_RegExec (FLUSH_RX);
	  /* leave while loop */
	  break;
	}

    };

  /* clear status */
  nRFCMD_ClearIRQ (MASK_IRQ_FLAGS);
  /* turn off LED */
  CONFIG_PIN_LED = 0;

  return res;
}

void
main (void)
{
  unsigned char i;

  /* configure CPU peripherals */
  OPTION = CONFIG_CPU_OPTION;
  TRISA = CONFIG_CPU_TRISA;
  TRISC = CONFIG_CPU_TRISC;
  WPUA = CONFIG_CPU_WPUA;
  ANSEL = CONFIG_CPU_ANSEL;
  CMCON0 = CONFIG_CPU_CMCON0;
  OSCCON = CONFIG_CPU_OSCCON;

  timer1_init ();

  sleep_jiffies (0xFFFF);

  nRFCMD_Init ();

  IOCA = IOCA | (1 << 0);

  for (i = 0; i < 20; i++)
    {
      CONFIG_PIN_LED = 1;
      sleep_jiffies (50 * TIMER1_JIFFIES_PER_MS);
      CONFIG_PIN_LED = 0;
      sleep_jiffies (50 * TIMER1_JIFFIES_PER_MS);
    }

  while (1)
    {
      /* prepare and send hello packet */
      if (protocol_setup_hello ())
	{
	  /* send away HELLO packet */
	  packet_tx ();
	  /* switch to rx mode */
	  switch_to_rxmode ();
	  /* listen for BOUNCERPKT_CMD_CHALLENGE_SETUP packets */
	  sleep_jiffies (50 * TIMER1_JIFFIES_PER_MS);
	  /* disable RX mode */
	  CONFIG_PIN_CE = 0;

	  if (packet_rx (BOUNCERPKT_CMD_CHALLENGE_SETUP))
	    {
	      protocol_calc_secret ();
	      protocol_setup_response ();
	      packet_tx ();
	    }

	  /* switch to TX mode */
	  switch_to_txmode ();
	  /* turn off LED */
	  CONFIG_PIN_LED = 0;
	}
    }

  // rest in peace
  while (1)
    sleep_jiffies (0xFFFF);
}
