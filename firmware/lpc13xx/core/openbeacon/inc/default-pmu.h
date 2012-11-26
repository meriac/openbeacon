/***************************************************************
 *
 * OpenBeacon.org - LPC13xx Power Management Functions
 *
 * Copyright 2011-2012 Milosch Meriac <meriac@openbeacon.de>
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

#ifndef __DEFAULT_PMU_H__
#define __DEFAULT_PMU_H__
#ifndef DISABLE_DEFAULT_PMU

extern void pmu_wait_ms (uint16_t ms);
extern void pmu_init (void);

#endif/*DISABLE_DEFAULT_PMU*/
#endif/*__DEFAULT_PMU_H__*/
