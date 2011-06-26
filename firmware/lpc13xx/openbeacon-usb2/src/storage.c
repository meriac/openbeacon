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

#define LOGFILE_STORAGE_SIZE (4*1024*1024)

void
storage_status (void)
{
#ifdef  ENABLE_FLASH
  static const uint8_t cmd_jedec_read_id = 0x9F;
  uint8_t rx[3];
  spi_txrx (SPI_CS_FLASH, &cmd_jedec_read_id, sizeof (cmd_jedec_read_id), rx,
	    sizeof (rx));
  /* Show FLASH ID */
  debug_printf (" * FLASH: ID:%02X-%02X-%02X\n", rx[0], rx[1], rx[2]);
#endif/*ENABLE_FLASH*/
}

#ifdef  ENABLE_FLASH
static void
storage_logfile_read_raw (uint32_t offset, uint32_t length, const void *src,
			  uint8_t * dst)
{
  (void) src;

  uint8_t tx[5];

  tx[0]=0x03; /* 25MHz Read */
  tx[1]=(uint8_t)(offset>>16);
  tx[2]=(uint8_t)(offset>> 8);
  tx[3]=(uint8_t)(offset);
  tx[4]=0x00;

  spi_txrx (SPI_CS_FLASH, tx, sizeof(tx), dst, length);
}
#endif/*ENABLE_FLASH*/

void
storage_init (uint16_t device_id)
{
  /* last entry in file chain is volume label */
  static const TDiskFile f_volume_label = {
    .name = DiskBPB.BS_VolLab,
  };

#ifdef  ENABLE_FLASH
  static char storage_logfile_name[] = "LOG-0000BIN";

  /* update string device id */
  storage_logfile_name[4] = hex_char ( device_id >> 12 );
  storage_logfile_name[5] = hex_char ( device_id >>  8 );
  storage_logfile_name[6] = hex_char ( device_id >>  4 );
  storage_logfile_name[7] = hex_char ( device_id >>  0 );

  static const TDiskFile f_logfile = {
    .length = LOGFILE_STORAGE_SIZE,
    .handler = storage_logfile_read_raw,
    .data = &f_logfile,
    .name = storage_logfile_name,
    .next = &f_volume_label,
  };
#endif/*ENABLE_FLASH*/

  /* read-me.htm file that redirects to project page */
  static const char readme[] =
    "<html><head><meta HTTP-EQUIV=\"REFRESH\" content=\"0; "
    "url=http://openbeacon.org/OpenBeacon_USB_2\"></head></html>";

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

  /* autorun.inf file that redirects to READ-ME.HTM */
  static const char autorun_inf[] =
    "[AutoRun]\n"
    "shellexecute=READ-ME.HTM\n";

  static const TDiskFile f_autorun = {
    .length = sizeof (autorun_inf) - 1,
    .handler = NULL,
    .data = &autorun_inf,
    .name = "AUTORUN INF",
    .next = &f_readme,
  };

  /* init virtual file system */
  vfs_init (&f_autorun);

#ifdef  ENABLE_FLASH
  /* setup SPI chipselect pin */
  spi_init_pin (SPI_CS_FLASH);
#endif/*ENABLE_FLASH*/
}

#endif /* DISK_SIZE>0 */
