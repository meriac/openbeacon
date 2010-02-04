/*
    OpenBeacon.org - OnAir protocol specification and definition
    Copyright 2006 Milosch Meriac <meriac@bitmanufaktur.de>

    Proximity protocol added by Ciro Cattuto <ciro.cattuto@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    This file was modified on: April 1st, 2009
*/

#ifndef __OPENBEACON_H__
#define __OPENBEACON_H__

#define CONFIG_TRACKER_CHANNEL 81
#define CONFIG_PROX_CHANNEL 76

#define XXTEA_BLOCK_COUNT 4

#define RFBPROTO_BEACONTRACKER  16
#define RFBPROTO_PROXTRACKER    42
#define RFBPROTO_PROXREPORT     69

#define RFBPROTO_BEACONTRACKER_VERSION 23

#define PROX_MAX 4

#define RFBFLAGS_ACK		0x01
#define RFBFLAGS_SENSOR		0x02
#define RFBFLAGS_INFECTED	0x04

#define OID_PERSON			0x0400
#define OID_HEALER			0x0200
#define OID_PERSON_MIN      1100

#define PACKED  __attribute__((__packed__))

typedef struct
{
  u_int8_t strength;
  u_int16_t oid_last_seen;
  u_int16_t powerup_count;
  u_int8_t reserved;
  u_int32_t seq;
} PACKED TSocioTracker;

typedef struct
{
  u_int16_t oid_prox[PROX_MAX];
  u_int16_t seq;
} PACKED TSocioProx;

typedef union
{
  TSocioProx prox;
  TSocioTracker tracker;
} PACKED TSocioPayload;

typedef struct
{
  u_int8_t proto;
  u_int16_t oid;
  u_int8_t flags;

  TSocioPayload p;

  u_int16_t crc;
} PACKED TSocioPatterns;

typedef struct
{
  u_int8_t proto;
  u_int8_t version, flags, strength;
  u_int32_t seq;
  u_int32_t oid;
  u_int16_t reserved;
  u_int16_t crc;
} PACKED TBeaconTracker;

typedef union
{
  u_int8_t proto;
  TSocioPatterns socio;
  TBeaconTracker beacon;
  u_int32_t block[XXTEA_BLOCK_COUNT];
  u_int8_t byte[XXTEA_BLOCK_COUNT * 4];
} PACKED TBeaconEnvelope;

typedef struct
{
  TBeaconEnvelope env;
  u_int32_t src_ip;
} PACKED TBeaconEnvelopeForwarded;


#endif/*__OPENBEACON_H__*/
