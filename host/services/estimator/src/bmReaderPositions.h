/*
    OpenBeacon.org - Reader Position Settings

    Copyright 2009 Milosch Meriac <meriac@bitmanufaktur.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    This file was modified on: April 1st, 2009
*/

#ifndef BMREADERPOSITIONS_H_
#define BMREADERPOSITIONS_H_

typedef struct
{
  uint32_t id, room, floor, group;
  double x, y;
} TReaderItem;

#define IPv4(a,b,c,d) ( ((uint32_t)a)<<24 | ((uint32_t)b)<<16 | ((uint32_t)c)<<8 | ((uint32_t)d)<<0 )

static const TReaderItem g_ReaderList[] = {
  // FH Wildau: group=2
  {IPv4 (10, 97, 180, 50), 1, 0, 2, 3.04, 01.44},
  {IPv4 (10, 97, 180, 51), 1, 0, 2, 03.18, 05.52},
  {IPv4 (10, 97, 180, 52), 1, 0, 2, 10.06, 05.42},
  {IPv4 (10, 97, 180, 53), 1, 0, 2, 10.06, 01.44},

  // Jacques Reader: group=1
  {IPv4 (64, 18, 93, 193), 1, 0, 1, 41.65, 39.10},
  {IPv4 (64, 18, 93, 194), 1, 0, 1, 11.65, 39.10},
  {IPv4 (64, 18, 93, 196), 1, 0, 1, 41.65, 9.10},
  {IPv4 (64, 18, 93, 197), 1, 0, 1, 11.65, 9.10},
  {IPv4 (64, 18, 93, 243), 2, 0, 1, 57.00, 14.00},
  {IPv4 (64, 18, 93, 244), 2, 0, 1, 57.00, 34.00},
  {IPv4 (64, 18, 93, 246), 2, 0, 1, 77.00, 34.00},
  {IPv4 (64, 18, 93, 247), 1, 0, 1, 26.65, 24.10},
  {IPv4 (64, 18, 93, 248), 2, 0, 1, 77.00, 14.00},
  {IPv4 (64, 18, 93, 195), 2, 0, 1, 67.00, 24.00}
};

#define READER_COUNT (sizeof(g_ReaderList)/sizeof(g_ReaderList[0]))

#endif
