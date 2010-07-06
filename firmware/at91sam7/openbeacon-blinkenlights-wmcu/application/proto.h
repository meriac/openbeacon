/***************************************************************
 *
 * OpenBeacon.org - OpenBeacon link layer protocol
 *
 * Copyright 2007 Milosch Meriac <meriac@openbeacon.de>
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

#ifndef __PROTO_H__
#define __PROTO_H__

#include "../../openbeacon-blinkenlights-dimmer/application/openbeacon.h"

#define NRF_POWERLEVEL_MAX 3

static inline unsigned short
PtSwapShort (unsigned short src)
{
  return (src >> 8) | (src << 8);
}

static inline unsigned long
PtSwapLong (unsigned long src)
{
  return (src >> 24) |
    (src << 24) | ((src >> 8) & 0x0000FF00) | ((src << 8) & 0x00FF0000);
}

extern void PtInitProtocol (void);
extern int PtSetFifoLifetimeSeconds (int Seconds);
extern int PtGetFifoLifetimeSeconds (void);
extern void PtDumpUIntToUSB (unsigned int data);
extern void PtDumpStringToUSB (const char *text);
extern void PtTransmit (BRFPacket * pkg, unsigned char broadcast);
extern void PtSetRfPowerLevel ( unsigned char Level );
extern unsigned char PtGetRfPowerLevel ( void );
extern void PtSetRfJamDensity ( unsigned char milliseconds );
extern unsigned char PtGetRfJamDensity ( void );
extern unsigned int rf_rec, rf_sent_broadcast, rf_sent_unicast;

#endif/*__PROTO_H__*/
