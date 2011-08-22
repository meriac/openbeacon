/***************************************************************
 *
 * OpenBeacon.org - main file for OpenPCD2 basic demo
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
#include "main.h"
#include "pmu.h"
#include "usbserial.h"

static void
rfid_hexdump (const void *buffer, int size)
{
  int i;
  const unsigned char *p = (unsigned char *) buffer;

  for (i = 0; i < size; i++)
    {
      if (i && ((i & 3) == 0))
	debug_printf (" ");
      debug_printf (" %02X", *p++);
    }
  debug_printf (" [size=%02i]\n", size);
}

static inline void
rfid_init_emulator (void)
{
  /* enable timer 32_0 */
  LPC_SYSCON->SYSAHBCLKCTRL |= EN_CT32B0;
  /* reset and stop counter */
  LPC_TMR32B0->TCR = 0x02;
  LPC_TMR32B0->TCR = 0x00;
  /* enable CT32B0_CAP0 timer clock */
  LPC_IOCON->PIO1_5 = 0x02;
  /* no capturing */
  LPC_TMR32B0->CCR = 0x00;
  /* no match */
  LPC_TMR32B0->MCR = 0;
  LPC_TMR32B0->EMR = 0;
  LPC_TMR32B0->PWMC = 0;
  /* incremented upon CAP0 input rising edge */
  LPC_TMR32B0->CTCR = 0x01;
  /* disable prescaling */
  LPC_TMR32B0->PR = 0;
  /* run counter */
  LPC_TMR32B0->TCR = 0x01;

  /* enable SVDD switch */
  rfid_write_register (0x6306, 0x2A);
  rfid_write_register (0x6307, 0x03);
  rfid_write_register (0x6106, 0x10);
  /* enable secure clock */
  rfid_write_register (0x6330, 0x80);
  rfid_write_register (0x6104, 0x00);
  /* output envelope to AUX1 */
#if 0
  rfid_write_register (0x6321, 0x34);
  rfid_write_register (0x6322, 0x0E);
#else
  rfid_write_register (0x6321, 0x32);
  rfid_write_register (0x6322, 0x02);
#endif
  rfid_write_register (0x6328, 0xF9);

  /* WTF? FIXME !!! */
  LPC_SYSCON->SYSAHBCLKCTRL |= EN_CT32B0;
}

static void
loop_rfid (void)
{
  int res, line, t;
  uint32_t counter;
  static unsigned char data[80];

  /* release reset line after 400ms */
  pmu_wait_ms (400);
  rfid_reset (1);
  /* wait for PN532 to boot */
  pmu_wait_ms (100);

  /* fully initialized */
  GPIOSetValue (LED_PORT, LED_BIT, LED_ON);

  /* read firmware revision */
  debug_printf ("\nreading firmware version...\n");
  data[0] = PN532_CMD_GetFirmwareVersion;
  while ((res = rfid_execute (&data, 1, sizeof (data))) < 0)
    {
      debug_printf ("Reading Firmware Version Error [%i]\n", res);
      pmu_wait_ms (450);
      GPIOSetValue (LED_PORT, LED_BIT, LED_ON);
      pmu_wait_ms (10);
      GPIOSetValue (LED_PORT, LED_BIT, LED_OFF);
    }

  debug_printf ("PN532 Firmware Version: ");
  if (data[1] != 0x32)
    rfid_hexdump (&data[1], data[0]);
  else
    debug_printf ("v%i.%i\n", data[2], data[3]);

  /* Set PN532 to virtual card */
  data[0] = PN532_CMD_SAMConfiguration;
  data[1] = 0x02;		/* Virtual Card Mode */
  data[2] = 0x00;		/* No Timeout Control */
  data[3] = 0x00;		/* No IRQ */
  if ((res = rfid_execute (&data, 4, sizeof (data))) > 0)
    {
      debug_printf ("SAMConfiguration: ");
      rfid_hexdump (&data, res);
    }

  /* init RFID emulator */
  rfid_init_emulator ();

  /* enable debug output */
  GPIOSetValue (LED_PORT, LED_BIT, LED_ON);
  line = 0;
  t = 0;

  while (1)
    {
      counter = LPC_TMR32B0->TC;
      pmu_wait_ms (500);
      counter = (LPC_TMR32B0->TC - counter)*2;

      debug_printf ("LPC_TMR32B0[%08u]: %uHz\n", line++, counter);
    }
}

int
main (void)
{
  /* Initialize GPIO (sets up clock) */
  GPIOInit ();

  /* Set LED port pin to output */
  GPIOSetDir (LED_PORT, LED_BIT, 1);
  GPIOSetValue (LED_PORT, LED_BIT, LED_OFF);

  /* Init RFID */
  rfid_init ();

  /* Init Power Management Routines */
  pmu_init ();

  /* UART setup */
  UARTInit (115200, 0);

  /* CDC USB Initialization */
  init_usbserial ();

  /* RUN RFID loop */
  loop_rfid ();

  return 0;
}
