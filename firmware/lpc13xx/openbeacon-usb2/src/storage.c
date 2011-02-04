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
} PACKED TDiskBPB;

void
storage_status (void)
{
  static const uint8_t cmd_jedec_read_id = 0x9F;
  uint8_t rx[3];
  spi_txrx (SPI_CS_FLASH, &cmd_jedec_read_id, sizeof (cmd_jedec_read_id), rx,
	    sizeof (rx));

  /* Show FLASH ID */
  debug_printf (" * FLASH: ID:%02X-%02X-%02X\n", rx[0], rx[1], rx[2]);
}


static void
msd_read_memory (uint32_t offset, uint32_t length, const void *src,
		 uint8_t * dst)
{
/*  debug_printf
    ("msd_read_memory: offset=0x%08X length=0x%04X src=0x%08X dst=0x%08X\n\n",
     offset, length, (uint32_t) src, (uint32_t) dst);*/

  memcpy (dst, ((const uint8_t*)src)+offset, length);
}

void
msd_read (uint32_t offset, uint8_t * dst, uint32_t length)
{
  const TDiskRecord *rec;
  uint32_t t, read_end, rec_start, rec_end, pos, count;
  uint8_t once;
  uint8_t *p;

  /* disk signature */
  static const uint32_t DiskSignature = 0x1B07CF6E;

  /* first partition table entry */
  static const TDiskPartitionTableEntry DiskPartitionTableEntry = {
    .bootable = 0x00,
    .start = {
	      .head = 0,
	      .sector = 2,
	      .cylinder = 0},
    .partition_type = 0x0C,
    .end = {
	    .head = DISK_HEADS - 1,
	    .sector = DISK_SECTORS_PER_TRACK,
	    .cylinder = DISK_CYLINDERS - 1},
    .start_lba = 1,
    .length = DISK_SECTORS - 1
  };

  /* MBR termination signature */
  static const uint16_t MBRSignature = 0xAA55;

  /* BPB - BIOS Parameter Block: actual volume boot block */
  static const TDiskBPB DiskBPB = {
    .BS_jmpBoot = {0xE9, 0x00, 0x00},
    .BS_OEMName = "BITMANUF"
  };

  /* data mapping of virtual drive */
  static const TDiskRecord DiskRecord[] = {
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
     .length = sizeof (MBRSignature),
     .handler = msd_read_memory,
     .data = &MBRSignature}
    ,
    /* BPB - BIOS Parameter Block: actual volume boot block */
    {
     .offset = DISK_BLOCK_SIZE,
     .length = sizeof (DiskBPB),
     .handler = msd_read_memory,
     .data = &DiskBPB}
  };

  /* ignore zero reads and reads outside of disk area */
  if (!length || (offset >= DISK_SIZE))
    return;

  /* truncate reads outside of disk area */
  if ((offset + length) > DISK_SIZE)
    length = DISK_SIZE - offset;

  /* FIXME - set memory to zero before filling up */
  memset (dst, 0, length);

  /* iterate DiskRecords and fill request with content */
  rec = DiskRecord;
  t = sizeof (DiskRecord) / sizeof (DiskRecord[0]);
  read_end = offset + length;

  once = 1;
  while (t--)
    {
      rec_start = rec->offset;
      rec_end = rec_start + rec->length;

      if ((read_end >= rec_start) && (offset < rec_end))
	{
/*	  if (once)
	    {
	      once = 0;
	      debug_printf
		("msd_read: offset=0x%08X length=0x%04X dst=0x%08X\n", offset,
		 length, (uint32_t) dst);
	    }
	  debug_printf ("     rec: offset=0x%08X length=0x%04X end=0x%08X\n",
			rec_start, rec->length, rec_end);*/

	  if(rec_start>=offset)
	  {
	    pos=0;
	    p=&dst[rec_start-offset];
	    count=(rec_end<=read_end)?rec->length:read_end-rec_start;
	  }
	  else
	  {
	    pos=offset-rec_start;
	    p=dst;
	    count=rec_end-offset;
	  }
	
	  rec->handler(pos,count,rec->data,p);
//	  debug_printf ("          pos=0x%03X count=0x%03X dst=0x%08X\n",pos,count,(uint32_t)p);

//          pos = 
/*	  if (IN_RANGE (offset, rec_start, rec_end))
	  {
	    pos = offset-rec_start;
	    rec->handler (pos, (read_end > rec_end) ? rec->length-pos : rec_end-offset,
			  rec->data, dst);
	  }
	  if (IN_RANGE (read_end, rec_start, rec_end))
	    {
	      pos = (rec_start < offset) ? offset - rec_start : 0;
	      rec->handler (pos,
			    (read_end >
			     rec_end) ? rec->length - pos : rec_end - offset,
			    rec->data, dst);
//          rec->handler (pos, rec->length - pos, rec->data, pos?dst:&dst[rec_start-offset]);
	    }*/
	}
      rec++;
    }
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
