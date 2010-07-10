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
#if SSP_DEBUG
#include "uart.h"
#endif

uint8_t src_addr[SSP_BUFSIZE];
uint8_t dest_addr[SSP_BUFSIZE];

/*****************************************************************************
** Function name:		LoopbackTest
**
** Descriptions:		Loopback test
**				
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void
LoopbackTest (void)
{
  uint32_t i;

#if !USE_CS
  /* Set SSEL pin to output low. */
  GPIOSetValue (PORT0, 2, 0);
#endif
  i = 0;
  while (i <= SSP_BUFSIZE)
    {
      /* to check the RXIM and TXIM interrupt, I send a block data at one time 
         based on the FIFOSIZE(8). */
      SSPSend ((uint8_t *) & src_addr[i], FIFOSIZE);
      /* If RX interrupt is enabled, below receive routine can be
         also handled inside the ISR. */
      SSPReceive ((uint8_t *) & dest_addr[i], FIFOSIZE);
      i += FIFOSIZE;
    }
#if !USE_CS
  /* Set SSEL pin to output high. */
  GPIOSetValue (PORT0, 2, 1);
#endif

  /* verifying write and read data buffer. */
  for (i = 0; i < SSP_BUFSIZE; i++)
    {
      if (src_addr[i] != dest_addr[i])
	{
	  while (1);		/* Verification failed */
	}
    }
  return;
}

/*****************************************************************************
** Function name:		SEEPROMTest
**
** Descriptions:		Serial EEPROM(Atmel 25xxx) test
**				
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void
SEEPROMTest (void)
{
  uint32_t i, timeout;
#if SSP_DEBUG
  uint8_t temp[2];
#endif

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
#if SSP_DEBUG
	  temp[0] = (dest_addr[i] & 0xF0) >> 4;
	  if ((temp[0] >= 0) && (temp[0] <= 9))
	    {
	      temp[0] += 0x30;
	    }
	  else
	    {
	      temp[0] -= 0x0A;
	      temp[0] += 0x41;
	    }
	  temp[1] = dest_addr[i] & 0x0F;
	  if ((temp[1] >= 0) && (temp[1] <= 9))
	    {
	      temp[1] += 0x30;
	    }
	  else
	    {
	      temp[1] -= 0x0A;
	      temp[1] += 0x41;
	    }
	  UARTSend ((uint8_t *) & temp[0], 2);
	  UARTSend ("\r\n", 2);
#endif
	  while (1);		/* Verification failed */
	}
    }
#if SSP_DEBUG
  UARTSend ("PASS\r\n", 6);
#endif
  return;
}

/******************************************************************************
**   Main Function  main()
******************************************************************************/
int
main (void)
{
  uint32_t i;

  SystemInit ();

#if SSP_DEBUG
  UARTInit (115200);
#endif

  SSPInit ();			/* initialize SSP port, share pins with SPI0
				   on port0(p0.15-18). */
  for (i = 0; i < SSP_BUFSIZE; i++)
    {
      src_addr[i] = (uint8_t) i;
      dest_addr[i] = 0;
    }

#if TX_RX_ONLY
  /* For the inter-board communication, one board is set as
     master transmit, the other is set to slave receive. */
#if SSP_SLAVE
  /* Slave receive */
  SSPReceive ((uint8_t *) dest_addr, SSP_BUFSIZE);
  for (i = 0; i < SSP_BUFSIZE; i++)
    {
      if (src_addr[i] != dest_addr[i])
	{
	  while (1);		/* Verification failure, fatal error */
	}
    }
#else
  /* Master transmit */
  SSPSend ((uint8_t *) src_addr, SSP_BUFSIZE);
#endif
#else
  /* TX_RX_ONLY=0, it's either an internal loopback test
     within SSP peripheral or communicate with a serial EEPROM. */
#if LOOPBACK_MODE
  LoopbackTest ();
#else
  SEEPROMTest ();
  /* If JTAG TCK is used as SSP clock, change the setting before
     serial EEPROM test, restore after the test. */
#ifdef __JTAG_DISABLED
  LPC_IOCON->JTAG_TCK_PIO0_10 &= ~0x07;	/* Restore JTAG_TCK */
#endif
#endif /* endif NOT LOOPBACK_MODE */
#endif /* endif NOT TX_RX_ONLY */
  return 0;
}

/******************************************************************************
**                            End Of File
******************************************************************************/
