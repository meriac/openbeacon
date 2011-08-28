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
  {IPv4 (10, 110, 0, 212), 102, 1, 10, 225,  13},
  {IPv4 (10, 110, 0, 211), 102, 1, 10, 505,  13},
  {IPv4 (10, 110, 0, 210), 102, 1, 10, 475,  13},
  {IPv4 (10, 110, 0, 209), 102, 1, 10, 385,  13},
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
  {IPv4 (10, 110, 0, 222), 303, 3, 10,  20, 230},

  // BCC: Hackcenter - Level A
  {IPv4 (10, 254, 3,   3), 101, 1,  2, 414, 434},
  {IPv4 (10, 254, 3,  11), 101, 1,  2, 188, 539},
  {IPv4 (10, 254, 3,  10), 101, 1,  2, 387, 681},
  {IPv4 (10, 254, 4,   6), 101, 1,  2, 742, 500},

  // BCC: Lounge - Level A
  {IPv4 (10, 254, 4,  13), 102, 1,  2, 497, 922},
  {IPv4 (10, 254, 4,  17), 102, 1,  2, 633, 832},

  // BCC: Hardware Lab - Level A
  {IPv4 (10, 254, 2,   7), 103, 1,  2, 370, 814},

  // BCC: Workshop - Level A
  {IPv4 (10, 254, 3,  21), 104, 1,  2, 212, 782},

  // BCC: Angel Heaven - Level A
  {IPv4 (10, 254, 5,   2), 105, 1,  2, 763, 195},

  // BCC: Foebud - Level B
  {IPv4 (10, 254, 5,  12), 201, 2,  2, 252, 387},

  // BCC: Helpdesk - Level B
  {IPv4 (10, 254, 5,  17), 202, 2,  2, 252, 640},

  // BCC: Entrance - Level B
  {IPv4 (10, 254, 6,   2), 203, 2,  2, 252, 798},

  // BCC: Checkroom - Level B
  {IPv4 (10, 254, 1,   7), 204, 2,  2, 664, 896},

  // BCC: Stairs Speakers - Level B
  {IPv4 (10, 254, 1,   1), 205, 2,  2, 734, 831},

  // BCC: Stairs CERT - Level B
  {IPv4 (10, 254, 4,  21), 206, 2,  2, 734, 342},

  // BCC: Saal 2 - Level B
  {IPv4 (10, 254, 3,   5), 207, 2,  2, 314,  78},
  {IPv4 (10, 254, 3,  15), 207, 2,  2,  92, 201},

  // BCC: Saal 3 - Level B
  {IPv4 (10, 254, 2,   5), 208, 2,  2, 549,  78},
  {IPv4 (10, 254, 2,  23), 208, 2,  2, 800,  78},
  {IPv4 (10, 254, 2, 150), 208, 2,  2, 662, 214},

  // BCC: Canteen - Level B
  {IPv4 (10, 254, 2,  12), 209, 2,  2, 412, 590},
  {IPv4 (10, 254, 2,   6), 209, 2,  2, 578, 590},
  {IPv4 (10, 254, 3,   1), 209, 2,  2, 573, 430},
  {IPv4 (10, 254, 2,  15), 209, 2,  2, 480, 734},

  // BCC: Saal 1 - Level C
  {IPv4 (10, 254, 6,  16), 301, 3,  2, 528, 520},
  {IPv4 (10, 254, 6,  22), 301, 3,  2, 528, 650},
  {IPv4 (10, 254, 5,  11), 301, 3,  2, 722, 432},
  {IPv4 (10, 254, 6,  21), 301, 3,  2, 722, 742},
  {IPv4 (10, 254, 0, 100), 301, 3,  2, 722, 586},

  // BCC: Debian - Level C
  {IPv4 (10, 254, 8,   1), 302, 3,  2, 426, 330},

  // BCC: Stairs Press - Level C
  {IPv4 (10, 254, 8,   5), 303, 3,  2, 669, 310},

  // BCC: Chaoswelle CAcert - Level C
  {IPv4 (10, 254, 8,  14), 304, 3,  2, 585, 249},

  // BCC: Wikipedia - Level C
  {IPv4 (10, 254, 7,  11), 305, 3,  2, 432, 838},
  {IPv4 (10, 254, 9,  13), 305, 3,  2, 465, 910},

  // BCC: POC Helpdesk VOIP - Level C
  {IPv4 (10, 254, 7,  17), 306, 3,  2, 292, 668},

  // BCC: NOC Helpdesk - Level C
  {IPv4 (10, 254, 7,  19), 307, 3,  2, 286, 492},

  // BCC: Stairs Hinterzimmer - Level C
  {IPv4 (10, 254, 9,  19), 308, 3,  2, 676, 858},
};

#define READER_COUNT (sizeof(g_ReaderList)/sizeof(g_ReaderList[0]))

#endif
