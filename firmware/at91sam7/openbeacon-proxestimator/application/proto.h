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

#define FIFO_DEPTH 256

typedef struct
{
	u_int16_t tag_oid;
	u_int8_t tag_strength;
	u_int8_t packet_count;
} __attribute__ ((packed)) TBeaconSort;

extern TBeaconEnvelope g_Beacon;

extern void vInitProtocolLayer (void);
extern int PtDebugLevel (int DebugLevel);
extern int PtSetFifoLifetimeSeconds (int Seconds);
extern int PtGetFifoLifetimeSeconds (void);
extern void tx_tag_command (unsigned int tag_id, unsigned int tag_id_new);
extern void wifi_tx_reader_command (unsigned int reader_id,
									unsigned char opcode,
									unsigned int data0, unsigned int data1);

#endif/*__PROTO_H__*/
