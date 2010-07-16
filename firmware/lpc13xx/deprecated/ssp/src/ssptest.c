/*****************************************************************************
 *   ssptest.c:  main C entry file for NXP LPC13xx Family Microprocessors
 *
 *   Copyright(C) 2008, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2008.07.19  ver 1.00    Preliminary version, first Release
 *
******************************************************************************/
#include "LPC13xx.h"		/* LPC13xx definitions */
#include "gpio.h"
#include "ssp.h"
#include "uart.h"
#include "debug_printf.h"

uint8_t src_addr[SSP_BUFSIZE];
uint8_t dest_addr[SSP_BUFSIZE];

void
SEEPROMTest (void)
{
  uint32_t i, timeout;

  LPC_IOCON->PIO0_2 &= ~0x07;	/* SSP SSEL is a GPIO pin */
  GPIOSetValue (PORT0, 2, 1);
  /* port0, bit 2 is set to GPIO output and high */
  GPIOSetDir (PORT0, 2, 1);

  GPIOSetValue (PORT0, 2, 0);
  /* Test Atmel 25016 SPI SEEPROM. */
  src_addr[0] = WREN;		/* set write enable latch */
  SSPSend ((uint8_t *) src_addr, 1);
  GPIOSetValue (PORT0, 2, 1);

  for (i = 0; i < DELAY_COUNT; i++);	/* delay minimum 250ns */

  GPIOSetValue (PORT0, 2, 0);
  src_addr[0] = RDSR;		/* check status to see if write enabled is latched */
  SSPSend ((uint8_t *) src_addr, 1);
  SSPReceive ((uint8_t *) dest_addr, 1);
  GPIOSetValue (PORT0, 2, 1);
  if ((dest_addr[0] & (RDSR_WEN | RDSR_RDY)) != RDSR_WEN)
    /* bit 0 to 0 is ready, bit 1 to 1 is write enable */
    {
      while (1);
    }

  for (i = 0; i < SSP_BUFSIZE; i++)	/* Init RD and WR buffer */
    {
      src_addr[i + 3] = i;	/* leave three bytes for cmd and offset(16 bits) */
      dest_addr[i] = 0;
    }

  /* please note the first two bytes of WR and RD buffer is used for
     commands and offset, so only 2 through SSP_BUFSIZE is used for data read,
     write, and comparison. */
  GPIOSetValue (PORT0, 2, 0);
  src_addr[0] = WRITE;		/* Write command is 0x02, low 256 bytes only */
  src_addr[1] = 0x00;		/* write address offset MSB is 0x00 */
  src_addr[2] = 0x00;		/* write address offset LSB is 0x00 */
  SSPSend ((uint8_t *) src_addr, SSP_BUFSIZE);
  GPIOSetValue (PORT0, 2, 1);

  for (i = 0; i < 0x30000; i++);	/* delay, minimum 3ms */

  timeout = 0;
  while (timeout < MAX_TIMEOUT)
    {
      GPIOSetValue (PORT0, 2, 0);
      src_addr[0] = RDSR;	/* check status to see if write cycle is done or not */
      SSPSend ((uint8_t *) src_addr, 1);
      SSPReceive ((uint8_t *) dest_addr, 1);
      GPIOSetValue (PORT0, 2, 1);
      if ((dest_addr[0] & RDSR_RDY) == 0x00)	/* bit 0 to 0 is ready */
	{
	  break;
	}
      timeout++;
    }
  if (timeout == MAX_TIMEOUT)
    {
      while (1);
    }

  for (i = 0; i < DELAY_COUNT; i++);	/* delay, minimum 250ns */

  GPIOSetValue (PORT0, 2, 0);
  src_addr[0] = READ;		/* Read command is 0x03, low 256 bytes only */
  src_addr[1] = 0x00;		/* Read address offset MSB is 0x00 */
  src_addr[2] = 0x00;		/* Read address offset LSB is 0x00 */
  SSPSend ((uint8_t *) src_addr, 3);
  SSPReceive ((uint8_t *) & dest_addr[3], SSP_BUFSIZE - 3);
  GPIOSetValue (PORT0, 2, 1);

  /* verifying, ignore the difference in the first two bytes */
  for (i = 3; i < SSP_BUFSIZE; i++)
    {
      if (src_addr[i] != dest_addr[i])
	{
	  while (1);		/* Verification failed */
	}
    }
  return;
}

/******************************************************************************
**   Main Function  main()
******************************************************************************/
int
main (void)
{
  SystemInit ();

  UARTInit (115200);

  SSPInit ();			/* initialize SSP port, share pins with SPI0
				   on port0(p0.15-18). */

  /* TX_RX_ONLY=0, it's either an internal loopback test
     within SSP peripheral or communicate with a serial EEPROM. */
  SEEPROMTest ();
  /* If JTAG TCK is used as SSP clock, change the setting before
     serial EEPROM test, restore after the test. */
  while(1)
  {
    debug_printf("Hello World!\n\r");
  }
  return 0;
}

/******************************************************************************
**                            End Of File
******************************************************************************/
