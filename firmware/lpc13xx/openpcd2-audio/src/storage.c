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

#ifdef  ENABLE_FLASH

static uint16_t g_device_id;

uint8_t
storage_flags (void)
{
	static const uint8_t cmd_read_status = 0x05;
	uint8_t status;

	spi_txrx (SPI_CS_FLASH, &cmd_read_status, sizeof (cmd_read_status),
			  &status, sizeof (status));

	return status;
}

static void
storage_prepare_write (void)
{
	static const uint8_t cmd_wren = 0x06;
	static const uint8_t cmd_en4b = 0xB7;

	/* allow write */
	do
	{
		spi_txrx (SPI_CS_FLASH, &cmd_wren, sizeof(cmd_wren), NULL, 0);
	} while ((storage_flags()&3)!=2);

	/* enable 32 bit adresses */
	spi_txrx (SPI_CS_FLASH, &cmd_en4b, sizeof(cmd_en4b), NULL, 0);
}

void
storage_erase (void)
{
	int i;
	uint8_t data;
	static const uint8_t cmd_erase = 0x60;

	storage_prepare_write ();

	/* erase flash */
	spi_txrx (SPI_CS_FLASH, &cmd_erase, sizeof(cmd_erase), NULL, 0);

	debug_printf("erasing flash (flags=0x%02X)...\n",storage_flags());

	i=0;
	while ( (data = storage_flags ()) & 1)
	{
		debug_printf("since %03is erasing [flags=%02X].\r",i++,data);
		pmu_wait_ms(1000);
	}

	debug_printf("\nerased flash...(flags=0x%02X)\n",storage_flags());
}

void
storage_read (uint32_t offset, uint32_t length, void *dst)
{
	uint8_t tx[5];

	tx[0] = 0x03;																/* 25MHz Read */
	tx[1] = (uint8_t) (offset >> 24);
	tx[2] = (uint8_t) (offset >> 16);
	tx[3] = (uint8_t) (offset >> 8);
	tx[4] = (uint8_t) (offset);

	spi_txrx (SPI_CS_FLASH, tx, sizeof (tx), dst, length);
}

void
storage_write (uint32_t offset, uint32_t length, const void *src)
{
	uint8_t tx[7];
	const uint8_t *p;

	if (length < 2)
		return;

	p = (const uint8_t *) src;

	tx[0] = 0x06;																/* Write Enable */
	spi_txrx (SPI_CS_FLASH, tx, 1, NULL, 0);

	/* write first block including address */
	tx[0] = 0xAD;																/* AAI Word Program */
	tx[1] = (uint8_t) (offset >> 24);
	tx[2] = (uint8_t) (offset >> 16);
	tx[3] = (uint8_t) (offset >> 8);
	tx[4] = (uint8_t) (offset);
	tx[5] = *p++;
	tx[6] = *p++;
	length -= 2;
	spi_txrx (SPI_CS_FLASH, tx, sizeof (tx), NULL, 0);

	while (length >= 2)
	{
		/* wait while busy */
		while (storage_flags () & 1);

		tx[1] = *p++;
		tx[2] = *p++;
		length -= 2;

		spi_txrx (SPI_CS_FLASH, tx, 3, NULL, 0);
	}

	/* wait while busy */
	while (storage_flags () & 1);

	tx[0] = 0x04;																/* Write Disable */
	spi_txrx (SPI_CS_FLASH, tx, 1, NULL, 0);
}

static void
storage_logfile_read_raw (uint32_t offset, uint32_t length, const void *write,
						  void * read)
{
	if (read)
		storage_read (offset, length, read);
	else
		if (write)
			storage_write (offset, length, write);
}
#endif /*ENABLE_FLASH */

void
storage_status (void)
{
#ifdef  ENABLE_FLASH
	static const uint8_t cmd_jedec_read_id = 0x9F;
	uint8_t rx[3];

	/* wait till ready */
	debug_printf ("Unlocking\n");
	storage_prepare_write ();

	spi_txrx (SPI_CS_FLASH, &cmd_jedec_read_id, sizeof (cmd_jedec_read_id),
			  rx, sizeof (rx));

	/* Show FLASH ID */
	debug_printf (" * FLASH:    ID=%02X-%02X-%02X\n", rx[0], rx[1], rx[2]);
#endif /*ENABLE_FLASH */
}

void
storage_init (uint16_t device_id)
{
	/* declare last entry in file chain is volume label */
	static const TDiskFile f_volume_label = {
		.name = DiskBPB.BS_VolLab,
	};

#ifdef  ENABLE_FLASH
	static char storage_logfile_name[] = "DATA0000BIN";
	static TDiskFile f_logfile = {
		.length = LOGFILE_STORAGE_SIZE,
		.handler = storage_logfile_read_raw,
		.data = &f_logfile,
		.name = storage_logfile_name,
		.next = &f_volume_label,
	};

	/* update string device id */
	g_device_id = device_id;
	storage_logfile_name[4] = hex_char (device_id >> 12);
	storage_logfile_name[5] = hex_char (device_id >> 8);
	storage_logfile_name[6] = hex_char (device_id >> 4);
	storage_logfile_name[7] = hex_char (device_id >> 0);
#endif /*ENABLE_FLASH */

	/* read-me.htm file that redirects to project page */
	static const char readme[] =
		"<html><head><meta HTTP-EQUIV=\"REFRESH\" content=\"0; "
		"url=http://openbeacon.org/OpenPCD2?ver="
		PROGRAM_VERSION "\"></head></html>";

	static const TDiskFile f_readme = {
		.length = sizeof (readme) - 1,
		.handler = NULL,
		.data = &readme,
		.name = "READ-ME HTM",
#ifdef  ENABLE_FLASH
		.next = &f_logfile,
#else /*ENABLE_FLASH */
		.next = &f_volume_label,
#endif /*ENABLE_FLASH */
	};

	/* version information */
	static const char version[] = PROGRAM_NAME ":v" PROGRAM_VERSION;

	static const TDiskFile f_version = {
		.length = sizeof (version) - 1,
		.handler = NULL,
		.data = &version,
		.name = "VERSION TXT",
		.next = &f_readme
	};

	/* autorun.inf file that redirects to READ-ME.HTM */
	static const char autorun_inf[] =
		"[AutoRun]\n" "shellexecute=READ-ME.HTM\n";

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
#endif /*ENABLE_FLASH */

	/* init virtual file system */
	vfs_init (&f_autorun);
}

#endif /* DISK_SIZE>0 */
