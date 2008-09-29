/***************************************************************
 *
 * OpenBeacon.org - OpenBeacon dimmer link layer protocol
 *
 * Copyright 2008 Milosch Meriac <meriac@openbeacon.de>
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

#ifndef __DIMMER_H__
#define __DIMMER_H__

#define DIMMER_TICKS 10000

extern void vInitDimmer (void);
extern void vUpdateDimmer (int Percent);
extern int dimmer_line_hz_enabled (void);
extern void vSetDimmerGamma (int entry, int val);
extern int vGetDimmerStep (void);
extern void vSetDimmerJitterUS (unsigned char us);
extern unsigned char vGetDimmerJitterUS (void);
extern int vGetEmiPulses (void);
extern void vSetDimmerOff (int off);
extern int vGetDimmerOff (void);

#endif/*__DIMMER_H__*/
