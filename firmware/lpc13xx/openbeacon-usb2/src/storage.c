/***************************************************************
 *
 * OpenBeacon.org - virtual FAT16 file system support
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
#include "vfs.h"
#include "storage.h"
#include "customIO.h"
#include "openbeacon-proto.h"

#if DISK_SIZE>0
#include "spi.h"

#ifdef  ENABLE_FLASH

#define LOGTXT_ENTRY_SIZE 34

static uint16_t g_device_id;
static uint32_t g_log_items;

uint32_t
storage_items (void)
{
  return g_log_items;
}

uint8_t
storage_flags (void)
{
  static const uint8_t cmd_read_status = 0x05;
  uint8_t status;

  spi_txrx (SPI_CS_FLASH, &cmd_read_status, sizeof (cmd_read_status), &status, sizeof(status));

  return status;
}

void
storage_erase (void)
{
  uint8_t tx[2];

  tx[0]=0x04; /* Write Disable */
  spi_txrx (SPI_CS_FLASH, tx, 1, NULL, 0);

  /* allow write */
  tx[0] = 0x06;
  spi_txrx (SPI_CS_FLASH, &tx, 1, NULL, 0);

  /* disable block protection */
  tx[0] = 0x01;
  tx[1] = 0x00;
  spi_txrx (SPI_CS_FLASH, &tx, 2, NULL, 0);

  /* allow write */
  tx[0] = 0x06;
  spi_txrx (SPI_CS_FLASH, &tx, 1, NULL, 0);

  /* erase flash */
  tx[0] = 0x60;
  spi_txrx (SPI_CS_FLASH, &tx, sizeof(tx[0]), NULL, 0);

  while(storage_flags() & 1);
}

void
storage_read (uint32_t offset, uint32_t length, void *dst)
{
  uint8_t tx[4];

  tx[0]=0x03; /* 25MHz Read */
  tx[1]=(uint8_t)(offset>>16);
  tx[2]=(uint8_t)(offset>> 8);
  tx[3]=(uint8_t)(offset);

  spi_txrx (SPI_CS_FLASH, tx, sizeof(tx), dst, length);
}

void
storage_write (uint32_t offset, uint32_t length, const void *src)
{
  uint8_t tx[6];
  const uint8_t *p;

  if(length<2)
    return;

  p = (const uint8_t*) src;

  tx[0]=0x06; /* Write Enable */
  spi_txrx (SPI_CS_FLASH, tx, 1, NULL, 0);

  /* write first block including address */
  tx[0]=0xAD; /* AAI Word Program */
  tx[1]=(uint8_t)(offset>>16);
  tx[2]=(uint8_t)(offset>> 8);
  tx[3]=(uint8_t)(offset);
  tx[4]=*p++;
  tx[5]=*p++;
  length-=2;
  spi_txrx (SPI_CS_FLASH, tx, sizeof(tx), NULL, 0);

  while(length>=2)
  {
    /* wait while busy */
    while(storage_flags() & 1);

    tx[1]=*p++;
    tx[2]=*p++;
    length-=2;

    spi_txrx (SPI_CS_FLASH, tx, 3, NULL, 0);
  }

  /* wait while busy */
  while(storage_flags() & 1);

  tx[0]=0x04; /* Write Disable */
  spi_txrx (SPI_CS_FLASH, tx, 1, NULL, 0);
}

static void
storage_scan_items (void)
{
  uint32_t t, pos;
  uint8_t *p;
  TLogfileBeaconPacket pkt;

  pos = 0;
  while (pos <= (LOGFILE_STORAGE_SIZE - sizeof (pkt)))
    {
      storage_read (pos, sizeof (pkt), &pkt);

      /* verify if block is empty */
      p = (uint8_t *) & pkt;
      for (t = 0; t < sizeof (pkt); t++)
	if ((*p++) != 0xFF)
	  break;

      /* return first empty block */
      if (t == sizeof (pkt))
	break;

      /* verify next block */
      pos += sizeof (pkt);
    }

  g_log_items = pos / sizeof(pkt);
}

static void
storage_logtxt_fmt (char* buffer, uint32_t index)
{
  uint32_t oid;
  TLogfileBeaconPacket pkt;

  if( index < g_log_items )
  {
    storage_read (index*sizeof(pkt), sizeof(pkt), &pkt);

    if(crc8((uint8_t*)&pkt, sizeof(pkt)-sizeof(pkt.crc)) == pkt.crc)
    {
      oid = ntohs(pkt.oid);

      if((pkt.strength & LOGFLAG_PROXIMITY) || (oid>9999))
	cIO_snprintf(buffer, LOGTXT_ENTRY_SIZE, "P%04X,%07u,P%04X,%u,%010u\n", g_device_id, ntohl(pkt.time), oid, pkt.strength & 0xF, ntohl(pkt.seq));
      else
	cIO_snprintf(buffer, LOGTXT_ENTRY_SIZE, "P%04X,%07u,T%04u,%u,%010u\n", g_device_id, ntohl(pkt.time), oid, pkt.strength & 0xF, ntohl(pkt.seq));
    }
    else
    {
      memset(buffer, ' ', LOGTXT_ENTRY_SIZE-2);
      buffer[LOGTXT_ENTRY_SIZE-2]='\r';
      buffer[LOGTXT_ENTRY_SIZE-1]='\n';
    }
  }
  else
    bzero(buffer, LOGTXT_ENTRY_SIZE);
}

static void
storage_logtxt_read_raw (uint32_t offset, uint32_t length, const void *src,
			  uint8_t * dst)
{
  uint32_t i, index, count;
  char buffer[LOGTXT_ENTRY_SIZE];
  (void) src;
  (void) offset;

  index = offset / LOGTXT_ENTRY_SIZE;

  /* handle remainder from previous block */
  if(((i = (offset % LOGTXT_ENTRY_SIZE))!=0) && (length>=i))
  {
    storage_logtxt_fmt(buffer, index++);
    count = LOGTXT_ENTRY_SIZE-i;
    memcpy(dst, &buffer[i], count);
    dst+=count;
    length-=count;
  }

  /* handle full bocks */
  while(length>=LOGTXT_ENTRY_SIZE)
  {
    storage_logtxt_fmt((char*)dst, index++);
    dst+=LOGTXT_ENTRY_SIZE;
    length-=LOGTXT_ENTRY_SIZE;
  }

  /* handle remainder of last block */
  if(length)
  {
    storage_logtxt_fmt(buffer, index);
    memcpy(dst, buffer, length);
  }
}

static void
storage_logfile_read_raw (uint32_t offset, uint32_t length, const void *src,
			  uint8_t * dst)
{
  (void) src;

  storage_read (offset, length, dst);
}
#endif/*ENABLE_FLASH*/

void
storage_status (void)
{
#ifdef  ENABLE_FLASH
  static const uint8_t cmd_jedec_read_id = 0x9F;
  uint8_t rx[3];
  spi_txrx (SPI_CS_FLASH, &cmd_jedec_read_id, sizeof (cmd_jedec_read_id), rx,
	    sizeof (rx));
  /* Show FLASH ID */
  debug_printf (" * FLASH:    ID=%02X-%02X-%02X\n", rx[0], rx[1], rx[2]);
  debug_printf (" *       Status=%02X\n", storage_flags());
#endif/*ENABLE_FLASH*/
}

void
storage_init (uint8_t usb_enabled, uint16_t device_id)
{
/* declare last entry in file chain is volume label */
static const TDiskFile f_volume_label = {
    .name = DiskBPB.BS_VolLab,
};

#ifdef  ENABLE_FLASH
  /* declare log file entry */
  static char storage_logtxt_name[]  = "LOG-0000CSV";
  static TDiskFile f_logtxt = {
    .handler = storage_logtxt_read_raw,
    .data = &f_logtxt,
    .name = storage_logtxt_name,
    .next = &f_volume_label,
  };

  static char storage_logfile_name[] = "LOG-0000BIN";
  static TDiskFile f_logfile = {
    .handler = storage_logfile_read_raw,
    .data = &f_logfile,
    .name = storage_logfile_name,
    .next = &f_logtxt,
  };

  /* update string device id */
  g_device_id = device_id;
  storage_logfile_name[4] = hex_char ( device_id >> 12 );
  storage_logfile_name[5] = hex_char ( device_id >>  8 );
  storage_logfile_name[6] = hex_char ( device_id >>  4 );
  storage_logfile_name[7] = hex_char ( device_id >>  0 );
  memcpy(&storage_logtxt_name[4],&storage_logfile_name[4],4);
#endif/*ENABLE_FLASH*/

  /* read-me.htm file that redirects to project page */
  static const char readme[] =
    "<html><head><meta HTTP-EQUIV=\"REFRESH\" content=\"0; "
    "url=http://openbeacon.org/OpenBeacon_USB_2?ver="
    PROGRAM_VERSION "\"></head></html>";

  static const TDiskFile f_readme = {
    .length = sizeof (readme) - 1,
    .handler = NULL,
    .data = &readme,
    .name = "READ-ME HTM",
#ifdef  ENABLE_FLASH
    .next = &f_logfile,
#else /*ENABLE_FLASH*/
    .next = &f_volume_label,
#endif/*ENABLE_FLASH*/
  };

  /* version information */
  static const char version[] =
    PROGRAM_NAME ":v" PROGRAM_VERSION;

  static const TDiskFile f_version = {
    .length = sizeof (version) - 1,
    .handler = NULL,
    .data = &version,
    .name = "VERSION TXT",
    .next = &f_readme
  };

  /* autorun.inf file that redirects to READ-ME.HTM */
  static const char autorun_inf[] =
    "[AutoRun]\n"
    "shellexecute=READ-ME.HTM\n";

  static const TDiskFile f_autorun = {
    .length = sizeof (autorun_inf) - 1,
    .handler = NULL,
    .data = &autorun_inf,
    .name = "AUTORUN INF",
    .next = &f_version,
  };

#ifdef  ENABLE_FLASH
  /* setup SPI chipselect pin */
  spi_init_pin (SPI_CS_FLASH);
  /* determine stored item count */
  storage_scan_items ();
  /* update file sizes */
  f_logfile.length = g_log_items * sizeof(TLogfileBeaconPacket);
  f_logtxt.length = g_log_items * LOGTXT_ENTRY_SIZE;
#endif/*ENABLE_FLASH*/

  /* init virtual file system */
  if(usb_enabled)
    vfs_init (&f_autorun);

}

#endif /* DISK_SIZE>0 */
