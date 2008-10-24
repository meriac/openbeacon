/*
    This file is part of the SocioPatterns firmware
    Copyright (C) 2008 Istituto per l'Interscambio Scientifico I.S.I.
    You can contact us by email (isi@isi.it) or write to:
    ISI Foundation, Viale S. Severo 65, 10133 Torino, Italy. 

    This program was written by Ciro Cattuto <ciro.cattuto@gmail.com>

    This program is based on:
    OpenBeacon.org - OnAir protocol specification and definition
    Copyright 2006 Milosch Meriac <meriac@openbeacon.de>

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

    This file was modified on: July 1st, 2008
*/

#ifndef __OPENBEACON_H__
#define __OPENBEACON_H__

#define RFBPROTO_BEACONTRACKER	24   // beacon to station, channel CONFIG_DEFAULT_CHANNEL
#define RFBPROTO_CONTACTTRACKER	42   // beacon to beacon, channel CONFIG_CONTACT_CHANNEL
#define RFBPROTO_CONTACTREPORT	69   // beacon to station, channel CONFIG_DEFAULT_CHANNEL

#define SEEN_MAX 4   // maximum number of reported contacts

#define TEA_ENCRYPTION_BLOCK_COUNT 4

#define RFB_RFOPTIONS		0x0F

#define RFBFLAGS_ACK		0x01
#define RFBFLAGS_SENSOR		0x02


typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned long u_int32_t;

bank1 typedef struct
{
	u_int8_t size, proto;
} TBeaconHeader;

bank1 typedef struct
{
	TBeaconHeader hdr;
	u_int16_t oid;
	u_int32_t seq;
	u_int8_t flags, strength;
	u_int16_t oid_last_seen;
	u_int16_t reserved;
	u_int16_t crc;
} TBeaconTracker;

bank1 typedef struct
{
	TBeaconHeader hdr;
	u_int16_t oid;
	u_int16_t oid_contact[SEEN_MAX];
	u_int16_t seq;
	u_int16_t crc;
} TBeaconContact;

bank1 typedef union
{
	TBeaconTracker pkt;
	TBeaconContact pkt_contact;
	u_int32_t data[TEA_ENCRYPTION_BLOCK_COUNT];
	u_int8_t datab[TEA_ENCRYPTION_BLOCK_COUNT * sizeof (u_int32_t)];
}
TBeaconEnvelope;

#endif /* __OPENBEACON_H__ */
