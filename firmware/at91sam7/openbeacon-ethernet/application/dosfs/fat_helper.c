/***************************************************************
 *
 * OpenBeacon.org - FAT file system helper functions
 *
 * Copyright 2009 Henryk Ploetz <henryk@ploetzli.ch>
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
#include <FreeRTOS.h>
#include <task.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "dosfs.h"
#include "debug_printf.h"

static VOLINFO vi;
static uint8_t scratch[SECTOR_SIZE], sector[SECTOR_SIZE];

/* Initialize a card, returns 0 on success. Uses global scratch buffer -> Not reentrant. */
int
fat_init (void)
{
  uint32_t pstart, psize;
  int res;
  uint8_t pactive, ptype;
  DIRINFO di;

  DFS_Init ();

  if ((res = DFS_OpenCard ()) != 0)
    {
      debug_printf ("Can't open SDCARD [%i]\n", res);
      return -ENODEV;
    }

  /* wait till card is ready */
  vTaskDelay (500 / portTICK_RATE_MS);

  pstart = DFS_GetPtnStart (0, scratch, 0, &pactive, &ptype, &psize);
  if (pstart == 0xffffffff)
    {
      if (DFS_GetVolInfo (0, scratch, 0, &vi))
	{
	  debug_printf ("Error getting volume information\n");
	  return -EIO;
	}
      else
	return -ENOENT;
    }
  else if (DFS_GetVolInfo (0, scratch, pstart, &vi))
    {
      debug_printf ("Error getting volume information from partition\n");
      return -EIO;
    }

  debug_printf ("Volume label '%-11.11s'\n", vi.label);

  debug_printf
    ("%d MB (%d clusters, %d sectors) in data area\nfilesystem IDd as ",
     (int) ((vi.numclusters * vi.secperclus / 2) / 1024),
     (int) vi.numclusters, (int) vi.numclusters * vi.secperclus);
  if (vi.filesystem == FAT12)
    debug_printf ("FAT12.\n");
  else if (vi.filesystem == FAT16)
    debug_printf ("FAT16.\n");
  else if (vi.filesystem == FAT32)
    debug_printf ("FAT32.\n");
  else
    debug_printf ("[unknown]\n");

  debug_printf
    ("%d sector/s per cluster\n%d reserved sector/s\nvolume total %d sectors\n",
     (int) vi.secperclus, (int) vi.reservedsecs, (int) vi.numsecs);
  debug_printf
    ("%d sectors per FAT\nfirst FAT at sector #%d\nroot dir at #%d.\n",
     (int) vi.secperfat, (int) vi.fat1, (int) vi.rootdir);
  debug_printf ("%d root dir entries\ndata area commences at sector #%d.\n",
		(int) vi.rootentries, (int) vi.dataarea);

  di.scratch = sector;
  if (DFS_OpenDir (&vi, (uint8_t *) "", &di))
    {
      debug_printf ("Error opening root directory\n");
      return -ENOENT;
    }
  else
    return 0;
}

uint32_t
fat_file_open (const char *filename, FILEINFO * fi)
{
  return DFS_OpenFile (&vi, filename, DFS_WRITE, scratch, fi);
}

uint32_t
fat_file_append (FILEINFO * fi, void *block, uint32_t length)
{
  uint32_t written;

  if (DFS_WriteFile (fi, sector, block, &written, length))
    written=0;

  return written;
}
