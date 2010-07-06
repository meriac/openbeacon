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

#include "openbeacon.h"
#include "usbshell.h"

#define PTINITNRFFRONTEND_RESETFIFO 0x01
#define PTINITNRFFRONTEND_INIT 0x02

extern void vInitProtocolLayer (unsigned char wmcu_id);
extern int PtSetFifoLifetimeSeconds (int Seconds);
extern int PtGetFifoLifetimeSeconds (void);
extern void PtInitNrfFrontend (int ResetType);
extern void PtDumpNrfRegisters (void);
extern void PtUpdateWmcuId (unsigned char id);
extern unsigned int packet_count;
extern unsigned int last_sequence;
extern unsigned int last_ping_seq;
extern unsigned int pings_lost;
extern unsigned int debug;

#endif/*__PROTO_H__*/
