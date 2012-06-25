/***************************************************************
 *
 * OpenBeacon.org - OnAir protocol specification and definition
 *
 * Copyright (C) 2008 Istituto per l'Interscambio Scientifico I.S.I.
 * extended by Ciro Cattuto <ciro.cattuto@gmail.com> by support for
 * SocioPatterns.org platform
 *
 * Copyright (C) 2006 Milosch Meriac <meriac@openbeacon.de>
 * Initial OpenBeacon.org tag data format specification
 *
/***************************************************************

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

#define CONFIG_TRACKER_CHANNEL 81
#define CONFIG_PROX_CHANNEL 76

#define XXTEA_BLOCK_COUNT 4

#define RFBPROTO_READER_COMMAND 23
#define RFBPROTO_BEACONTRACKER  24
#define RFBPROTO_PROXTRACKER    42
#define RFBPROTO_PROXREPORT     69
#define RFBPROTO_PROXREPORT_EXT 70

#define PROX_MAX 4
#define PROX_TAG_ID_BITS 12
#define PROX_TAG_COUNT_BITS 2
#define PROX_TAG_STRENGTH_BITS 2
#define PROX_TAG_ID_MASK ((1<<PROX_TAG_ID_BITS)-1)
#define PROX_TAG_COUNT_MASK ((1<<PROX_TAG_COUNT_BITS)-1)
#define PROX_TAG_STRENGTH_MASK ((1<<PROX_TAG_STRENGTH_BITS)-1)

#define RFBFLAGS_ACK		0x01
#define RFBFLAGS_SENSOR		0x02
#define RFBFLAGS_INFECTED	0x04
#define RFBFLAGS_OID_WRITE	0x08
#define RFBFLAGS_OID_UPDATED	0x10
#define RFBFLAGS_WRAPPED_SEQ	0x20

#define OID_PERSON			0x0400
#define OID_HEALER			0x0200
#define OID_PERSON_MIN      1100

typedef struct
{
	u_int8_t proto;
	u_int16_t oid;
	u_int8_t flags;
} TBeaconHeader;

typedef struct
{
	TBeaconHeader hdr;
	u_int8_t strength;
	u_int16_t oid_last_seen;
	u_int16_t powerup_count;
	u_int8_t reserved;
	u_int32_t seq;
	u_int16_t crc;
} TBeaconTracker;

typedef struct
{
	TBeaconHeader hdr;
	u_int16_t oid_prox[PROX_MAX];
	u_int16_t short_seq;
	u_int16_t crc;
} TBeaconProx;

typedef union
{
	TBeaconHeader hdr;
	TBeaconTracker tracker;
	TBeaconProx prox;
	u_int32_t block[XXTEA_BLOCK_COUNT];
	u_int8_t byte[XXTEA_BLOCK_COUNT * 4];
}
TBeaconEnvelope;

#endif/*__OPENBEACON_H__*/
