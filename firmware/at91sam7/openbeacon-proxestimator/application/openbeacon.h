/***************************************************************
 *
 * OpenBeacon.org - OnAir protocol specification and definition
 *
 * Copyright 2006 Milosch Meriac <meriac@openbeacon.de>
 * Copyright 2008 Ciro Cattuto <ciro.cattuto@gmail.com>
 *
 ***************************************************************/

/*
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

#ifndef __OPENBEACON_H__
#define __OPENBEACON_H__

#define TEA_ENCRYPTION_BLOCK_COUNT 4

#define RFB_RFOPTIONS		0x0F

#define RFBPROTO_BEACONTRACKER	24
#define RFBPROTO_CONTACTTRACKER	42
#define RFBPROTO_CONTACTREPORT	69

#define RFBFLAGS_ACK		0x01
#define RFBFLAGS_SENSOR		0x02

#define PACKED  __attribute__((__packed__))

typedef struct
{
  u_int8_t proto;
  u_int16_t oid;
  u_int8_t size;
  u_int8_t flags, strength;
  u_int16_t oid_last_seen, reserved;
  u_int32_t seq;
  u_int16_t crc;
} PACKED TBeaconTracker;

typedef struct
{
  u_int8_t proto;
  u_int16_t oid;
  u_int16_t oid_contact[3];
  u_int8_t count;
  u_int32_t seq;
  u_int16_t crc;
} PACKED TBeaconContact;

typedef union
{
  TBeaconTracker pkt;
  TBeaconContact pkt_contact;
  u_int32_t data[TEA_ENCRYPTION_BLOCK_COUNT];
  u_int8_t datab[TEA_ENCRYPTION_BLOCK_COUNT * sizeof (u_int32_t)];
} PACKED TBeaconEnvelope;

#endif/*__OPENBEACON_H__*/
