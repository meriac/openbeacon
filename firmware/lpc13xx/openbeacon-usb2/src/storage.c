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

#if DISK_SIZE>0
#include "spi.h"

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

void
storage_init (void)
{
  /* last entry in file chain is volume label */
  static const TDiskFile f_volume_label = {
    .name = DiskBPB.BS_VolLab,
  };

  static const TDiskFile f_benchmark = {
    .length = 100 * 1024 * 1024,
    .handler = NULL,
    .data = NULL,
    .name = "TRANSFERIMG",
    .next = &f_volume_label,
  };

  /* readme.htm file that redirects to project page */
  static const char hello_world[]=
    "<html><head><meta HTTP-EQUIV=\"REFRESH\" content=\"0; "
    "url=http://openbeacon.org/OpenBeacon_USB_2\"></head></html>";

  static const TDiskFile f_readme = {
    .length = sizeof(hello_world)-1,
    .handler = NULL,
    .data = &hello_world,
    .name = "READ-ME HTM",
    .next = &f_benchmark,
  };

  /* init virtual file system */
  vfs_init (&f_readme);

  /* setup SPI chipselect pin */
  spi_init_pin (SPI_CS_FLASH);
}

#endif /* DISK_SIZE>0 */
