/***************************************************************
 *
 * OpenBeacon.org - OpenBeacon dimmer link layer protocol
 *
 * Copyright 2008 Milosch Meriac <meriac@openbeacon.de>
 *                Daniel Mack <daniel@caiaq.de>
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

#include <beacontypes.h>
#include "rnd.h"

static u_int32_t random_seed_v1 = 0x52f7d319;
static u_int32_t random_seed_v2 = 0x6e28014a;

u_int32_t
RndNumber (void)
{
  // MWC generator, period length 1014595583
  return ((random_seed_v1 =
	   36969 * (random_seed_v1 & 0xffff) +
	   (random_seed_v1 >> 16)) << 16) ^ (random_seed_v2 =
					     30963 *
					     (random_seed_v2 & 0xffff) +
					     (random_seed_v2 >> 16));
}

void
vRndInit ( u_int32_t seed )
{
  /* set the random seed */
  random_seed_v1 += seed;
  random_seed_v2 -= seed;
}
