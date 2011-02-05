/***************************************************************
 *
 * OpenBeacon.org - FLASH storage support
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
#include "msd.h"
#include "storage.h"

#if DISK_SIZE>0
#include "spi.h"

#define IN_RANGE(x,a,b) ((x>=a) && (x<b))
#define VOLUME_START 1UL
#define VOLUME_SECTORS (DISK_SECTORS-VOLUME_START)
#define DISK_SECTORS_PER_CLUSTER 8UL
#define FAT16_SIZE_SECTORS ((uint32_t)((((VOLUME_SECTORS/DISK_SECTORS_PER_CLUSTER+2)*2)+DISK_BLOCK_SIZE-1)/DISK_BLOCK_SIZE))
#define RESERVED_SECTORS_COUNT 1UL
#define BPB_NUMFATS 2UL
#define ROOT_DIR_SECTORS 1UL
#define MAX_FILES_IN_ROOT ((ROOT_DIR_SECTORS*DISK_BLOCK_SIZE)/32)
#define FIRST_FAT_SECTOR (RESERVED_SECTORS_COUNT)
#define FIRST_ROOT_DIR_SECTOR (FIRST_FAT_SECTOR+(BPB_NUMFATS*FAT16_SIZE_SECTORS))
#define FIRST_DATA_SECTOR (FIRST_ROOT_DIR_SECTOR+ROOT_DIR_SECTORS)
#define DATA_SECTORS (DISK_SECTORS-FIRST_DATA_SECTOR)
#define FIRST_SECTOR_OF_CLUSTER(N) (((N-2UL)*DISK_SECTORS_PER_CLUSTER)+FIRST_DATA_SECTOR)

typedef void (*TDiskHandler) (uint32_t offset, uint32_t length,
			      const void *src, uint8_t * dst);

typedef struct
{
  uint32_t offset, length;
  TDiskHandler handler;
  const void *data;
} TDiskRecord;

typedef struct
{
  uint8_t head, sector, cylinder;
} PACKED TDiskPartitionTableEntryCHS;

typedef struct
{
  uint8_t bootable;
  TDiskPartitionTableEntryCHS start;
  uint8_t partition_type;
  TDiskPartitionTableEntryCHS end;
  uint32_t start_lba;
  uint32_t length;
} PACKED TDiskPartitionTableEntry;

typedef struct
{
  uint8_t BS_jmpBoot[3];
  char BS_OEMName[8];
  uint16_t BPB_BytsPerSec;
  uint8_t BPB_SecPerClus;
  uint16_t BPB_RsvdSecCnt;
  uint8_t BPB_NumFATs;
  uint16_t BPB_RootEntCnt;
  uint16_t BPB_TotSec16;
  uint8_t BPB_Media;
  uint16_t BPB_FATSz16;
  uint16_t BPB_SecPerTrk;
  uint16_t BPB_NumHeads;
  uint32_t BPB_HiddSec;
  uint32_t BPB_TotSec32;
  /* FAT12/FAT16 definition */
  uint8_t BS_DrvNum;
  uint8_t BS_Reserved1;
  uint8_t BS_BootSig;
  uint32_t BS_VolID;
  char BS_VolLab[11];
  char BS_FilSysType[8];
} PACKED TDiskBPB;

static uint32_t debug_level = 0;

void
storage_status (void)
{
  static const uint8_t cmd_jedec_read_id = 0x9F;
  uint8_t rx[3];
  spi_txrx (SPI_CS_FLASH, &cmd_jedec_read_id, sizeof (cmd_jedec_read_id), rx,
	    sizeof (rx));

  /* enable debug output */
  debug_level = 1;

  /* Show FLASH ID */
  debug_printf (" * FLASH: ID:%02X-%02X-%02X\n", rx[0], rx[1], rx[2]);
}

static void
msd_read_data_area (uint32_t offset, uint32_t length, const void *src,
		    uint8_t * dst)
{
  (void) src;
  (void) offset;
  (void) dst;
  (void) length;
  memset (dst, 0, length);
}

static void
msd_read_fat_area (uint32_t offset, uint32_t length, const void *src,
		   uint8_t * dst)
{
  (void) src;
  (void) offset;
  (void) dst;
  (void) length;
  memset (dst, 0, length);
}

static void
msd_read_root_dir (uint32_t offset, uint32_t length, const void *src,
		   uint8_t * dst)
{
  (void) src;
  (void) offset;
  (void) dst;
  (void) length;
  memset (dst, 0, length);
}

static void
msd_read_memory (uint32_t offset, uint32_t length, const void *src,
		 uint8_t * dst)
{
  memcpy (dst, ((const uint8_t *) src) + offset, length);
}

void
msd_read (uint32_t offset, uint8_t * dst, uint32_t length)
{
  const TDiskRecord *rec;
  uint32_t t, read_end, rec_start, rec_end, pos, count, written;
  uint8_t *p;

  /* disk signature */
  static const uint32_t DiskSignature = 0x1B07CF6E;

  /* first partition table entry */
  static const TDiskPartitionTableEntry DiskPartitionTableEntry = {
    .bootable = 0x00,
    .start = {
	      .head = 0,
	      .sector = VOLUME_START + 1,
	      .cylinder = 0},
    .partition_type = 0x0C,
    .end = {
	    .head = DISK_HEADS - 1,
	    .sector = DISK_SECTORS_PER_TRACK,
	    .cylinder = DISK_CYLINDERS - 1},
    .start_lba = 1,
    .length = VOLUME_SECTORS
  };

  /* MBR termination signature */
  static const uint16_t BootSignature = 0xAA55;

  /* BPB - BIOS Parameter Block: actual volume boot block */
  static const TDiskBPB DiskBPB = {
    .BS_jmpBoot = {0xEB, 0x00, 0x90},
    .BS_OEMName = "BEACON01",
    .BPB_BytsPerSec = DISK_BLOCK_SIZE,
    .BPB_SecPerClus = DISK_SECTORS_PER_CLUSTER,
    .BPB_RsvdSecCnt = RESERVED_SECTORS_COUNT,
    .BPB_NumFATs = BPB_NUMFATS,
    .BPB_RootEntCnt = MAX_FILES_IN_ROOT,
    .BPB_Media = 0xF8,
    .BPB_FATSz16 = FAT16_SIZE_SECTORS,
    .BPB_SecPerTrk = DISK_SECTORS_PER_TRACK,
    .BPB_NumHeads = DISK_HEADS,
    .BPB_HiddSec = VOLUME_START,
    .BPB_TotSec32 = VOLUME_SECTORS,
    /* FAT12/FAT16 definition */
    .BS_DrvNum = 0x80,
    .BS_BootSig = 0x29,
    .BS_VolID = 0xe9d9489f,
    .BS_VolLab = "OpenBeacon",
    .BS_FilSysType = "FAT16   ",
  };

  /* data mapping of virtual drive */
  static const TDiskRecord DiskRecord[] = {
    /* data area */
    {
     .offset = (VOLUME_START + FIRST_DATA_SECTOR) * DISK_BLOCK_SIZE,
     .length = DATA_SECTORS * DISK_BLOCK_SIZE,
     .handler = msd_read_data_area,
     .data = NULL}
    ,
    /* FAT area - primary copy */
    {
     .offset = (VOLUME_START + FIRST_FAT_SECTOR) * DISK_BLOCK_SIZE,
     .length = FAT16_SIZE_SECTORS * DISK_BLOCK_SIZE,
     .handler = msd_read_fat_area,
     .data = NULL}
    ,
    /* FAT area - secondary copy */
    {
     .offset =
     (VOLUME_START + FIRST_FAT_SECTOR + FAT16_SIZE_SECTORS) * DISK_BLOCK_SIZE,
     .length = FAT16_SIZE_SECTORS * DISK_BLOCK_SIZE,
     .handler = msd_read_fat_area,
     .data = NULL}
    ,
    /* root dir  */
    {
     .offset = (VOLUME_START + FIRST_ROOT_DIR_SECTOR) * DISK_BLOCK_SIZE,
     .length = ROOT_DIR_SECTORS * DISK_BLOCK_SIZE,
     .handler = msd_read_root_dir,
     .data = NULL}
    ,
    /* disk signature */
    {
     .offset = 0x1B8,
     .length = sizeof (DiskSignature),
     .handler = msd_read_memory,
     .data = &DiskSignature}
    ,
    /* first partition table entry */
    {
     .offset = 0x1BE,
     .length = sizeof (DiskPartitionTableEntry),
     .handler = msd_read_memory,
     .data = &DiskPartitionTableEntry}
    ,
    /* MBR termination signature */
    {
     .offset = 0x1FE,
     .length = sizeof (BootSignature),
     .handler = msd_read_memory,
     .data = &BootSignature}
    ,
    /* BPB - BIOS Parameter Block: actual volume boot block */
    {
     .offset = (VOLUME_START * DISK_BLOCK_SIZE),
     .length = sizeof (DiskBPB),
     .handler = msd_read_memory,
     .data = &DiskBPB}
    ,
    /* BPB termination signature */
    {
     .offset = (VOLUME_START * DISK_BLOCK_SIZE) + 0x1FE,
     .length = sizeof (BootSignature),
     .handler = msd_read_memory,
     .data = &BootSignature}
    ,
  };

  /* ignore zero reads and reads outside of disk area */
  if (!length || (offset >= DISK_SIZE))
    return;

  /* iterate DiskRecords and fill request with content */
  rec = DiskRecord;
  t = sizeof (DiskRecord) / sizeof (DiskRecord[0]);
  read_end = offset + length;
  written = 0;

  while (t--)
    {
      rec_start = rec->offset;
      rec_end = rec_start + rec->length;

      if ((read_end >= rec_start) && (offset < rec_end))
	{
	  if (rec_start >= offset)
	    {
	      pos = 0;
	      p = &dst[rec_start - offset];
	      count = (rec_end <= read_end) ?
		rec->length : read_end - rec_start;
	    }
	  else
	    {
	      pos = offset - rec_start;
	      p = dst;
	      count = (read_end > rec_end) ? rec_end - offset : length;
	    }

	  /* set memory of partial read to zero before filling up */
	  if (!written && (count != length))
	    memset (dst, 0, length);

	  /* handle request */
	  rec->handler (pos, count, rec->data, p);
	  written += count;

	  /* all bytes handled -> quit */
	  if (written == length)
	    break;
	}
      rec++;
    }

  /* set memory to zero if not written yet  */
  if (!written)
    memset (dst, 0, length);
}

void
msd_write (uint32_t offset, uint8_t * src, uint32_t length)
{
  (void) offset;
  (void) src;
  (void) length;
}

void
storage_init (void)
{
  /* init USB mass storage */
  msd_init ();
  /* setup SPI chipselect pin */
  spi_init_pin (SPI_CS_FLASH);
}

#endif /* DISK_SIZE>0 */
