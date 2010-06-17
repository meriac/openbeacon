/***************************************************************
 *
 * OpenBeacon.org - Reader Position Settings
 *
 * Copyright 2009 Milosch Meriac <meriac@bitmanufaktur.de>
 *
 ***************************************************************/

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation; version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
  // HTW: group 1
  {IPv4 (10, 1, 254, 100), 707, 7, 1, 15.5, 5.5},
  {IPv4 (10, 1, 254, 103), 707, 7, 1, 0.5, 0.5},
  {IPv4 (10, 1, 254, 104), 707, 7, 1, 0.5, 5.5},
  {IPv4 (10, 1, 254, 105), 707, 7, 1, 8.0, 3.0},
  {IPv4 (10, 1, 254, 106), 707, 7, 1, 15.5, 0.5},

  // FH Wildau: group=10
  {IPv4 (10, 110, 0, 214), 101, 1, 10, 685, 290},
  {IPv4 (10, 110, 0, 213), 101, 1, 10, 685, 150},
  {IPv4 (10, 110, 0, 212), 102, 1, 10, 225, 13},
  {IPv4 (10, 110, 0, 211), 102, 1, 10, 505, 13},
  {IPv4 (10, 110, 0, 210), 102, 1, 10, 475, 13},
  {IPv4 (10, 110, 0, 209), 102, 1, 10, 385, 13},
  {IPv4 (10, 110, 0, 208), 103, 1, 10, 230, 172},
  {IPv4 (10, 110, 0, 236), 103, 1, 10, 230, 390},
  {IPv4 (10, 110, 0, 241), 101, 1, 10, 620, 290},
  {IPv4 (10, 110, 0, 221), 201, 2, 10, 600, 410},
  {IPv4 (10, 110, 0, 220), 201, 2, 10, 365, 410},
  {IPv4 (10, 110, 0, 219), 203, 2, 10, 140, 405},
  {IPv4 (10, 110, 0, 218), 203, 2, 10, 140, 100},
  {IPv4 (10, 110, 0, 217), 202, 2, 10, 365, 150},
  {IPv4 (10, 110, 0, 216), 202, 2, 10, 600, 150},
  {IPv4 (10, 110, 0, 215), 204, 2, 10, 730, 485},
  {IPv4 (10, 110, 0, 230), 304, 3, 10, 620, 410},
  {IPv4 (10, 110, 0, 229), 304, 3, 10, 390, 410},
  {IPv4 (10, 110, 0, 228), 303, 3, 10, 111, 410},
  {IPv4 (10, 110, 0, 227), 303, 3, 10, 110, 410},
  {IPv4 (10, 110, 0, 226), 302, 3, 10, 390, 145},
  {IPv4 (10, 110, 0, 225), 302, 3, 10, 620, 145},
  {IPv4 (10, 110, 0, 223), 301, 3, 10, 750, 233},
  {IPv4 (10, 110, 0, 222), 303, 3, 10, 20, 230},
};

#define READER_COUNT (sizeof(g_ReaderList)/sizeof(g_ReaderList[0]))

#endif
