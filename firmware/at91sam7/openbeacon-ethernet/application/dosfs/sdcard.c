/***************************************************************
 *
 * OpenBeacon.org - OpenBeacon SDCARD protocol implementation
 *                  for integrated MicroSDHC drive
 *
 * MMC/SDSC/SDHC (in SPI mode) control module  (C)ChaN, 2007
 *
 * adapted to AT91SAM7 OpenBeacon/OpenPICC2 by
 * Milosch Meriac <meriac@openbeacon.de>
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
#include <AT91SAM7.h>
#include <board.h>
#include <errno.h>
#include <string.h>
#include <task.h>
#include <stdio.h>
#include <stdint.h>

#include "sdcard.h"
#include "spi.h"
#include "dosfs.h"

/* Definitions for MMC/SDC command */
#define CMD0	(0x40+0)	/* GO_IDLE_STATE */
#define CMD1	(0x40+1)	/* SEND_OP_COND (MMC) */
#define	ACMD41	(0xC0+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(0x40+8)	/* SEND_IF_COND */
#define CMD9	(0x40+9)	/* SEND_CSD */
#define CMD10	(0x40+10)	/* SEND_CID */
#define CMD12	(0x40+12)	/* STOP_TRANSMISSION */
#define ACMD13	(0xC0+13)	/* SD_STATUS (SDC) */
#define CMD16	(0x40+16)	/* SET_BLOCKLEN */
#define CMD17	(0x40+17)	/* READ_SINGLE_BLOCK */
#define CMD18	(0x40+18)	/* READ_MULTIPLE_BLOCK */
#define CMD23	(0x40+23)	/* SET_BLOCK_COUNT (MMC) */
#define	ACMD23	(0xC0+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(0x40+24)	/* WRITE_BLOCK */
#define CMD25	(0x40+25)	/* WRITE_MULTIPLE_BLOCK */
#define CMD55	(0x40+55)	/* APP_CMD */
#define CMD58	(0x40+58)	/* READ_OCR */

/* Disk Status Bits (DSTATUS) */
#define STA_NOINIT		0x01	/* Drive not initialized */
#define STA_NODISK		0x02	/* No medium in the drive */
#define STA_PROTECT		0x04	/* Write protected */

/* Card type flags (CardType) */
#define CT_MMC			0x01		/* MMC ver 3 */
#define CT_SD1			0x02		/* SD ver 1 */
#define CT_SD2			0x04		/* SD ver 2 */
#define CT_SDC			(CT_SD1|CT_SD2)	/* SD */
#define CT_BLOCK		0x08		/* Block addressing */

/* Results of Disk Functions */
typedef enum
{
  RES_OK = 0,			/* 0: Successful */
  RES_ERROR,			/* 1: R/W Error */
  RES_WRPRT,			/* 2: Write Protected */
  RES_NOTRDY,			/* 3: Not Ready */
  RES_PARERR			/* 4: Invalid Parameter */
} DRESULT;

#define SD_CMD_TIMEOUT 50

/* Port Controls  (Platform dependent) */
#define SELECT()   AT91F_PIO_ClearOutput (SDCARD_CS_PIO, SDCARD_CS_PIN)
#define DESELECT() AT91F_PIO_SetOutput (SDCARD_CS_PIO, SDCARD_CS_PIN)

/* FIXME: What's the correct CPOL/NCPHA setting? */
#define SDCARD_SPI_CONFIG(scbr) (AT91C_SPI_BITS_8 | AT91C_SPI_CPOL | \
	(scbr << 8) | (1L << 16) | (0L << 24))

static const int SCBR_INIT = ((int) (MCK / 4e5) + 1) & 0xFF;
static const int SCBR = 3;
static spi_device sdcard_spi;
static volatile int Stat = STA_NOINIT;	/* Disk status */
static uint8_t CardType;	/* b0:MMC, b1:SDv1, b2:SDv2, b3:Block addressing */
static int claim_count = 0;


static inline uint8_t
rcvr_spi (void)
{
  uint8_t data = 0xFF;
  spi_transceive_blocking (&sdcard_spi, &data, sizeof (data));
  return data;
}

/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/

static uint8_t
wait_ready (void)
{
  uint8_t res = 0;
  int timeout = 255;

  res = rcvr_spi ();

  while ((res != 0xff) && timeout--)
    res = rcvr_spi ();

  return res;
}

/*-----------------------------------------------------------------------*/
/* Deselect the card and release SPI bus                                 */
/*-----------------------------------------------------------------------*/

static void
sdcard_claim (void)
{
  if (claim_count > 0)
    {
      claim_count++;
      return;
    }

  int res;
  while ((res = spi_start_bus_exclusive (&sdcard_spi)) == -EAGAIN)
    vTaskDelay (10 / portTICK_RATE_MS);

  claim_count++;
}

static void
sdcard_release (void)
{
  claim_count--;
  if (claim_count > 0)
    {
      return;
    }

  DESELECT ();
  rcvr_spi ();

  /* Put card into idle state by sending CMD0 */
  spi_stop_bus_exclusive (&sdcard_spi);

}

/*-----------------------------------------------------------------------*/
/* Receive a data packet from MMC                                        */
/*-----------------------------------------------------------------------*/

static inline void
sdcard_transceive (uint8_t * buff,	/* Data buffer to store received data */
		   uint32_t btr	/* Byte count (must be even number) */
  )
{
  if (btr <= 512)
    {
      /* For short transceive lengths, do a blocking transceive to get around
       * the multiple context switchs and interrupts associated with a standard
       * background transceive.
       */
      spi_transceive_blocking (&sdcard_spi, buff, btr);
    }
  else
    {
      spi_transceive (&sdcard_spi, buff, btr);
      spi_wait_for_completion (&sdcard_spi);
    }
}


static int
sdcard_block_read (uint8_t * buff,	/* Data buffer to store received data */
		   uint32_t btr	/* Byte count (must be even number) */
  )
{
  uint8_t token, timeout = 255;

  do
    {				/* Wait for data packet in timeout of 100ms */
      token = rcvr_spi ();
      if (token != 0xFF)
	break;
    }
  while (timeout--);
  if (token != 0xFE)
    {
      return 0;			/* If not valid data token, return with error */
    }

  spi_force_transmit_pin (&sdcard_spi, 1);
  sdcard_transceive (buff, btr);

  uint8_t scratch[2];
  sdcard_transceive (scratch, sizeof (scratch));	/* Discard CRC */

  spi_release_transmit_pin (&sdcard_spi);

  return 1;			/* Return with success */
}

/*-----------------------------------------------------------------------*/
/* Send a command packet to MMC                                          */
/*-----------------------------------------------------------------------*/

static int
sdcard_send_cmd (uint8_t cmd,	/* Command byte */
		 uint32_t arg	/* Argument */
  )
{
  int res, n;
  uint8_t data[7];

  if (cmd & 0x80)
    {				/* ACMD<n> is the command sequence of CMD55-CMD<n> */
      cmd &= 0x7F;
      res = sdcard_send_cmd (CMD55, 0);
      if (res > 1)
	return res;
    }

  /* Select the card and wait for ready */
  DESELECT ();
  SELECT ();
  if (cmd != CMD0 && wait_ready () != 0xff)
    {
      return 0xff;
    }

  /* Send command packet */
  data[0] = 0xFF;
  data[1] = cmd;
  data[2] = (uint8_t) (arg >> 24);
  data[3] = (uint8_t) (arg >> 16);
  data[4] = (uint8_t) (arg >> 8);
  data[5] = (uint8_t) (arg >> 0);
  switch (cmd)
    {
    case (CMD0 & 0x7F):
      data[6] = 0x95;
      break;
    case (CMD8 & 0x7F):
      data[6] = 0x87;
      break;
    default:
      data[6] = 0xFF;
    }
  sdcard_transceive (data, sizeof (data));

  /* Receive command response */
  if (cmd == CMD12)
    rcvr_spi ();		/* Skip a stuff byte when stop reading */
  n = 50;			/* Wait for a valid response in timeout of 50 attempts */
  do
    {
      res = rcvr_spi ();
    }
  while (((res & 0x80) || (res == 0x1f) || (res == 0x3f)) && --n);

  return res;			/* Return with the response value */
}

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

int
DFS_OpenCard (void)
{
  uint8_t cmd, ty, ocr[10];
  uint32_t timeout = 10000;

  sdcard_claim ();

  SELECT ();
  memset (ocr, 0xFF, sizeof (ocr));
  sdcard_transceive (ocr, sizeof (ocr));
  DESELECT ();
  memset (ocr, 0xFF, 2);
  sdcard_transceive (ocr, sizeof (ocr));

  ty = 0;
  if (sdcard_send_cmd (CMD0, 0) == 1)
    {				/* Enter Idle state */
      if (sdcard_send_cmd (CMD8, 0x1AA) == 1)
	{			/* SDHC */
	  sdcard_transceive (ocr, 4);

	  if (ocr[2] == 0x01 && ocr[3] == 0xAA)
	    {			/* The card can work at vdd range of 2.7-3.6V */
	      while (timeout-- && sdcard_send_cmd (ACMD41, 1UL << 30));	/* Wait for leaving idle state (ACMD41 with HCS bit) */
	      if (timeout && sdcard_send_cmd (CMD58, 0) == 0)
		{		/* Check CCS bit in the OCR */
		  sdcard_transceive (ocr, 4);
		  ty = (ocr[0] & 0x40) ? 12 : 4;
		}
	    }
	}
      else
	{			/* SDSC or MMC */
	  if (sdcard_send_cmd (ACMD41, 0) <= 1)
	    {
	      ty = 2;
	      cmd = ACMD41;	/* SDSC */
	    }
	  else
	    {
	      ty = 1;
	      cmd = CMD1;	/* MMC */
	    }
	  while (timeout-- && sdcard_send_cmd (cmd, 0));	/* Wait for leaving idle state */
	  if (!timeout || sdcard_send_cmd (CMD16, 512) != 0)	/* Set R/W block length to 512 */
	    ty = 0;
	}
    }
  CardType = ty;
  //  if(ty == 12)
  //    spi_change_config (&sdcard_spi, SDCARD_SPI_CONFIG (SCBR));
  sdcard_release ();

  debug_printf ("SD Card type %i detected\n", ty);

  if (ty)			/* Initialization succeded */
    Stat &= ~STA_NOINIT;	/* Clear STA_NOINIT */

  return Stat;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

static inline int
sdcard_disk_read (uint8_t * buff,	/* Pointer to the data buffer to store read data */
		  uint32_t sector,	/* Start sector number (LBA) */
		  uint32_t block_count	/* Number of blocks to read */
  )
{
  int res = RES_ERROR;

  if (Stat & STA_NOINIT)
    return RES_NOTRDY;

  if (!(CardType & CT_BLOCK))
    sector *= SECTOR_SIZE;	/* Convert to byte address if needed */

  sdcard_claim ();

  if (block_count == 1)
    {
      if ((sdcard_send_cmd (CMD17, sector) == 0)	/* READ_SINGLE_BLOCK */
	  && sdcard_block_read (buff, SECTOR_SIZE))
	res = RES_OK;
    }
  else
    {
      if ((sdcard_send_cmd (CMD18, sector) == 0))
	{			/* READ_MULTIPLE_BLOCK */
	  do
	    {
	      if (!sdcard_block_read (buff, SECTOR_SIZE))
		{
		  break;
		}
	      buff += SECTOR_SIZE;
	    }
	  while (--block_count);
	  sdcard_send_cmd (CMD12, 0);
	  if (block_count == 0)
	    res = RES_OK;
	}
    }

  sdcard_release ();

  return res;
}

uint32_t
DFS_ReadSector (uint8_t unit, uint8_t * buffer, uint32_t sector,
		uint32_t count)
{
  (void) unit;

  if (sdcard_disk_read (buffer, sector, count) != RES_OK)
    return 1;

  return 0;
}


static int
xmit_datablock (			/* 1:OK, 0:Failed */
		 uint8_t * buff,	/* 512 byte data block to be transmitted */
		 uint8_t token		/* Data/Stop token */
  )
{
  uint8_t d[2];

  if (!wait_ready ())
    return 0;

  /* Xmit a token */
  spi_transceive_blocking (&sdcard_spi, &token, sizeof(token));
  /* Is it data token? */
  if (token != 0xFD)
    {
					/* Xmit the 512 byte data block to MMC */
					/* Dummy CRC (FF,FF) */
      spi_transceive_blocking (&sdcard_spi, buff, 512);
      spi_transceive_blocking (&sdcard_spi, &d, sizeof (d));
      if ((rcvr_spi() & 0x1F) != 0x05)	/* If not accepted, return with error */
	return 0;
    }
  return 1;
}

static inline int
sdcard_disk_write (uint8_t * buff,	/* Pointer to the data to be written */
		   uint32_t sector,	/* Start sector number (LBA) */
		   uint8_t count	/* Sector count (1..128) */
  )
{

  if (Stat & STA_NOINIT)
    return RES_NOTRDY;
  if (!count)
    return RES_PARERR;
  if (!(CardType & CT_BLOCK))
    sector *= SECTOR_SIZE;		/* Convert LBA to byte address if needed */

  sdcard_claim ();

  if (count == 1)
    {				/* Single block write */
      if ((sdcard_send_cmd (CMD24, sector) == 0)	/* WRITE_BLOCK */
	  && xmit_datablock (buff, 0xFE))
	count = 0;
    }
  else
    {				/* Multiple block write */
      if (CardType & CT_SDC)
	sdcard_send_cmd (ACMD23, count);
      if (sdcard_send_cmd (CMD25, sector) == 0)
	{			/* WRITE_MULTIPLE_BLOCK */
	  do
	    {
	      if (!xmit_datablock (buff, 0xFC))
		break;
	      buff += 512;
	    }
	  while (--count);
	  if (!xmit_datablock (0, 0xFD))	/* STOP_TRAN token */
	    count = 1;
	}
    }

  sdcard_release ();

  return count ? RES_ERROR : RES_OK;
}

uint32_t
DFS_WriteSector (uint8_t unit, uint8_t * buffer, uint32_t sector,
		 uint32_t count)
{
  (void) unit;

  if (sdcard_disk_write (buffer, sector, count) != RES_OK)
    return 1;

  return 0;
}

int
DFS_Init (void)
{
  AT91F_PIO_CfgOutput (SDCARD_CS_PIO, SDCARD_CS_PIN);
  AT91F_PIO_SetOutput (SDCARD_CS_PIO, SDCARD_CS_PIN);

  spi_open (&sdcard_spi, SDCARD_CS, SDCARD_SPI_CONFIG (SCBR_INIT));

  return 0;
}
