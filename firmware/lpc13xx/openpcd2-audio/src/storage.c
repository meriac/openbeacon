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
#include "beacon_db.h"

#if DISK_SIZE>0
#include "spi.h"

#ifdef  ENABLE_FLASH

#define SECTOR_BUFFER_SIZE 256

#define MAX_DIR_ENTRIES 128
static uint16_t g_device_id;
static TBeaconDbDirEntry g_dbdir[MAX_DIR_ENTRIES],g_dbdir_entry;
static int g_dbdir_count;
#define DIR_ENTRY_SIZE sizeof(g_dbdir[0])

static uint8_t g_sector_buffer[SECTOR_BUFFER_SIZE];
static uint32_t g_sector;

/* declare last entry in file chain is volume label */
static const TDiskFile f_volume_label = {
	.name = DiskBPB.BS_VolLab,
};
static TDiskFile f_database_file;

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
	int i,t;
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

		for(t=0;t<10;t++)
		{
			GPIOSetValue (LED1_PORT, LED1_BIT, LED_ON);
			pmu_wait_ms(10);
			GPIOSetValue (LED1_PORT, LED1_BIT, LED_OFF);
			pmu_wait_ms(90);
		}
	}

	debug_printf("\nerased flash...(flags=0x%02X)\n",storage_flags());
}

void
storage_read (uint32_t offset, uint32_t length, void *dst)
{
	uint8_t tx[6];

	tx[0] = 0x0B;																/* Fast Read */
	tx[1] = (uint8_t) (offset >> 24);
	tx[2] = (uint8_t) (offset >> 16);
	tx[3] = (uint8_t) (offset >> 8);
	tx[4] = (uint8_t) (offset);
	tx[5] = 0;

	spi_txrx (SPI_CS_FLASH, tx, sizeof (tx), dst, length);
}

void
storage_flush (void)
{
	uint8_t tx[5];

	/* wait while busy */
	while (storage_flags () & 1);

	tx[0] = 0x06;															/* Write Enable */
	spi_txrx (SPI_CS_FLASH, tx, 1, NULL, 0);

	/* write first block including address */
	tx[0] = 0x02;															/* Page Program */
	tx[1] = (uint8_t) (g_sector >> 24);
	tx[2] = (uint8_t) (g_sector >> 16);
	tx[3] = (uint8_t) (g_sector >> 8);
	tx[4] = (uint8_t) (g_sector);

	spi_txrx (SPI_CS_FLASH|SPI_CS_MODE_SKIP_CS_DEASSERT, tx, sizeof (tx), NULL, 0);
	spi_txrx (SPI_CS_FLASH, g_sector_buffer, sizeof(g_sector_buffer), NULL, 0);
}

void
storage_write (uint32_t offset, uint32_t length, const void *src)
{
	int pos;

	/* position inside sector buffer */
	pos = (uint8_t)offset;

	if((offset>>8)!=(g_sector>>8))
	{
		debug_printf("SEEK: @0x%08X -> 0x%08X\n",g_sector, offset);
		storage_flush();

		/* reset buffer */
		g_sector = offset & ~0xFFUL;
		memset(g_sector_buffer, 0, sizeof(g_sector_buffer));
	}

	if((length+pos)>SECTOR_BUFFER_SIZE)
	{
		debug_printf("WRITE: Overflow 0x%08X [%u:%u]\n",g_sector, pos, length);
		length = SECTOR_BUFFER_SIZE-pos;
	}

	memcpy(&g_sector_buffer[pos],src,length);
	pos+=length;
	if(pos>=SECTOR_BUFFER_SIZE)
	{
		storage_flush();
		g_sector+=SECTOR_BUFFER_SIZE;
	}
}

static void
storage_logfile_read_raw (uint32_t offset, uint32_t length, const void *write,
						  void * read)
{
	GPIOSetValue (LED1_PORT, LED1_BIT, LED_OFF);
	if (read)
		storage_read (offset, length, read);

	if (write)
		storage_write (offset, length, write);
	GPIOSetValue (LED1_PORT, LED1_BIT, LED_ON);
}
#endif /*ENABLE_FLASH */

void
storage_status (void)
{
#ifdef  ENABLE_FLASH
	static const uint8_t cmd_jedec_read_id = 0x9F;
	uint8_t rx[3];

	/* reset directory */
	g_dbdir_count = 0;
	memset(&g_dbdir_entry,0,sizeof(g_dbdir_entry));
	memset(&g_dbdir,0,sizeof(g_dbdir));
	storage_read(0,sizeof(g_dbdir[0]),&g_dbdir);

	/* wait till ready */
	debug_printf ("Unlocking\n");
	storage_prepare_write ();

	spi_txrx (SPI_CS_FLASH, &cmd_jedec_read_id, sizeof (cmd_jedec_read_id),
			  rx, sizeof (rx));

	/* Show FLASH ID */
	debug_printf (" * FLASH:    ID=%02X-%02X-%02X\n", rx[0], rx[1], rx[2]);

	if(g_dbdir[0].type != DB_DIR_ENTRY_TYPE_ROOT)
		debug_printf (" * FLASH: invalid entry type\n");
	else
	{
		int len, pos, crc;
		len = ntohl(g_dbdir[0].length);
		pos = ntohl(g_dbdir[0].pos);
		crc = ntohs(g_dbdir[0].crc16);
		debug_printf (" * FLASH ROOT @0x%04X[size:0x%04X, CRC=0x%04X]\n",pos,len,crc);
		if(len>(int)sizeof(g_dbdir))
			debug_printf (" * FLASH root directory overflow\n");
		{
			g_dbdir_count = len/DIR_ENTRY_SIZE;

			storage_read(pos,len,&g_dbdir);

			if(icrc16((uint8_t*)&g_dbdir,len)!=crc)
				debug_printf(" * FLASH ROOT CRC16 ERROR !!!\n");
			else
			{
				debug_printf(" * FLASH ROOT CRC16 OK [%i entries]\n",g_dbdir_count);
			}
		}
	}
#endif /*ENABLE_FLASH */
}

int
storage_db_find (uint8_t type, uint16_t id)
{
	int i;
	TBeaconDbDirEntry *p;

	memset(&g_dbdir_entry,0,sizeof(g_dbdir_entry));

	p = g_dbdir;
	id = htons(id);
	for(i=0;i<g_dbdir_count;i++)
		if((p->type == type) && (p->id == id))
		{
			g_dbdir_entry.type = p->type;
			g_dbdir_entry.flags = p->flags;
			g_dbdir_entry.id = ntohs(p->id);
			g_dbdir_entry.next_id = ntohs(p->next_id);
			g_dbdir_entry.pos = ntohl(p->pos);
			g_dbdir_entry.length = ntohl(p->length);
			g_dbdir_entry.crc16 = ntohs(p->crc16);

			return g_dbdir_entry.length;
		}
		else
			p++;

	return -1;
}

int
storage_db_read (void* buffer, uint32_t size)
{
	if(size>g_dbdir_entry.length)
		size=g_dbdir_entry.length;

	if(size)
	{
		storage_read(g_dbdir_entry.pos, size, buffer);
		/* adjust remaining pos & size */
		g_dbdir_entry.pos+=size;
		g_dbdir_entry.length-=size;
	}
	return size;
}

void
storage_connect (uint8_t enabled_db)
{
	if(!enabled_db)
		f_database_file.next = &f_volume_label;

	msd_connect (TRUE);
}

void
storage_init (uint16_t device_id, uint8_t connect)
{
#ifdef  ENABLE_FLASH
	static char storage_logfile_name[] = "DATA0000BIN";
	static TDiskFile f_logfile = {
		.length = LOGFILE_STORAGE_SIZE,
		.handler = storage_logfile_read_raw,
		.data = &f_logfile,
		.name = storage_logfile_name,
		.next = &f_volume_label,
	};

	/* reset sector positions */
	g_sector = 0;

	/* update string device id */
	g_device_id = device_id;

	if(device_id)
	{
		storage_logfile_name[4] = hex_char (device_id >> 12);
		storage_logfile_name[5] = hex_char (device_id >> 8);
		storage_logfile_name[6] = hex_char (device_id >> 4);
		storage_logfile_name[7] = hex_char (device_id >> 0);
	}
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

	/* copy file descriptor into RAM to allow changes in storage_connect() */
	f_database_file = f_readme;

	/* version information */
	static const char version[] = PROGRAM_NAME ":v" PROGRAM_VERSION;

	static const TDiskFile f_version = {
		.length = sizeof (version) - 1,
		.handler = NULL,
		.data = &version,
		.name = "VERSION TXT",
		.next = &f_database_file,
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

	GPIOSetDir (FLASH_RESET_PORT, FLASH_RESET_PIN, 1);
	GPIOSetValue (FLASH_RESET_PORT, FLASH_RESET_PIN, 1);
	LPC_IOCON->JTAG_TMS_PIO1_0 = 1;
#endif /*ENABLE_FLASH */

	/* init virtual file system */
	if(device_id)
		vfs_init (&f_autorun, connect);
}

#endif /* DISK_SIZE>0 */
