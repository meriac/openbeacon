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
#ifndef SDCARD_H_
#define SDCARD_H_

extern int DFS_Init (void);
extern int DFS_OpenCard (void);
extern uint32_t DFS_ReadSector  (uint8_t unit, uint8_t * buffer, uint32_t sector, uint32_t count);
extern uint32_t DFS_WriteSector (uint8_t unit, uint8_t * buffer, uint32_t sector, uint32_t count);

#endif /*SDCARD_H_ */
