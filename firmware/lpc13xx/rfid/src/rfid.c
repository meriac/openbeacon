#include <LPC13xx.h>
#include <string.h>
#include <config.h>
#include <gpio.h>
#include <uart.h>
#include <debug_printf.h>


#define PN532_ACK_NACK_SIZE 6

#define RESET_PORT 0
#define RESET_PIN 0
#define CS_PORT 0
#define CS_PIN 2

#define BIT_REVERSE(x) ((unsigned char)(__RBIT(x)>>24))

static unsigned char
rfid_tx (unsigned char data)
{
  while ((LPC_SSP->SR & 0x02) == 0);
  LPC_SSP->DR = BIT_REVERSE (data);
  while ((LPC_SSP->SR & 0x04) == 0);
  data = BIT_REVERSE (LPC_SSP->DR);
  return data;
}

static void
rfid_cs (unsigned char cs)
{
  GPIOSetValue (CS_PORT, CS_PIN, cs);
}

static unsigned char
rfid_status (void)
{
  unsigned char res;

  /* enable chip select */
  rfid_cs (0);

  /* transmit status request */
  rfid_tx (0x02);
  res = rfid_tx (0x00);

  /* release chip select */
  rfid_cs (1);

  return res;
}

static void
rfid_wait_response (void)
{
  /* wait till PN532 response is ready */
  while ((rfid_status () & 1) == 0);
}

static void
rfid_read (unsigned char *data, unsigned char size)
{
  /* wait for PN532 response */
  rfid_wait_response ();

  /* enable chip select */
  rfid_cs (0);

  /* read from FIFO command */
  rfid_tx (0x03);
  while (size--)
    *data++ = rfid_tx (0x00);

  /* release chip select */
  rfid_cs (1);
}

static void
rfid_reset (unsigned char reset)
{
  GPIOSetValue (RESET_PORT, RESET_PIN, reset);
}

static unsigned char
rfid_write (const void *data, int len)
{
  int i;
  static const unsigned char preamble[] = { 0x01, 0x00, 0x00, 0xFF };
  static const unsigned char ack[PN532_ACK_NACK_SIZE] =
    { 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00 };
  unsigned char response[PN532_ACK_NACK_SIZE];
  const unsigned char *p = preamble;
  unsigned char tfi = 0xD4, c;

  /* enable chip select */
  rfid_cs (0);

  p = preamble;			/* Praeamble */
  for (i = 0; i < (int) sizeof (preamble); i++)
    rfid_tx (*p++);
  rfid_tx (len + 1);		/* LEN */
  rfid_tx (0x100 - (len + 1));	/* LCS */
  rfid_tx (tfi);		/* TFI */
  /* PDn */
  p = (const unsigned char *) data;
  while (len--)
    {
      c = *p++;
      rfid_tx (c);
      tfi += c;
    }
  rfid_tx (0x100 - tfi);	/* DCS */
  rfid_tx (0x00);		/* Postamble */

  /* release chip select */
  rfid_cs (1);

  /* eat ack */
  rfid_read (response, sizeof (response));

  return memcmp (response, ack, PN532_ACK_NACK_SIZE) == 0;
}

void
rfid_init (void)
{
  int i;
  unsigned char data[13];

  debug_printf ("Hello RFID!\n");

  /* reset SSP peripheral */
  LPC_SYSCON->PRESETCTRL = 0x01;

  /* Enable SSP clock */
  LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 11);

  // Enable SSP peripheral
  LPC_IOCON->PIO0_8 = 0x01 | (0x01 << 3);	/* MISO, Pulldown */
  LPC_IOCON->PIO0_9 = 0x01;	/* MOSI */

  LPC_IOCON->SCKLOC = 0x00;	/* route to PIO0_10 */
  LPC_IOCON->JTAG_TCK_PIO0_10 = 0x02;	/* SCK */

  /* Set SSP clock to 4.5MHz */
  LPC_SYSCON->SSPCLKDIV = 0x01;
  LPC_SSP->CR0 = 0x0707;
  LPC_SSP->CR1 = 0x0002;
  LPC_SSP->CPSR = 0x02;

  /* Initialize chip select line */
  rfid_cs (1);
  GPIOSetDir (CS_PORT, CS_PIN, 1);

  /* Initialize RESET line */
  rfid_reset (0);
  GPIOSetDir (RESET_PORT, RESET_PIN, 1);
  for (i = 0; i < 10000; i++);
  rfid_reset (1);

  while (1)
    {
      /* read firmware revision */
      debug_printf ("\nreading firmware version...\n");
      data[0] = 0x02;
      if (rfid_write (&data, 1))
	{
	  debug_printf ("getting result\n");
	  rfid_read (data, sizeof (data));
	  for (i = 0; i < (int)sizeof (data); i++)
	    debug_printf (" %02X", data[i]);
	  debug_printf ("\n");
	}
      else
	debug_printf ("->NACK!\n");
    }
}
