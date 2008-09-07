/***************************************************************
 *
 * OpenBeacon.org - OnAir OpenBouncer protocol definition
 *
 * Copyright 2006 Milosch Meriac <meriac@openbeacon.de>
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

#ifndef __OPENBOUNCER_H__
#define __OPENBOUNCER_H__

#define BOUNCERPKT_VERSION 0x01
#define BOUNCERPKT_PICKS_COUNT 8
#define BOUNCERPKT_PICKS_LIST_SIZE 32

#define BOUNCERPKT_CMD_HELLO           0x01
#define BOUNCERPKT_CMD_CHALLENGE_SETUP 0x02
#define BOUNCERPKT_CMD_CHALLENGE       0x03
#define BOUNCERPKT_CMD_RESPONSE        0x04

typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned long u_int32_t;

/*
 * PROTOCOL ENDIANESS = Network Byte Order
 */ 

/* 
 * TBouncerHeader
 *
 * default header common for all packets
 *
 */
typedef struct {
  u_int8_t version;
  u_int8_t command;
  u_int8_t value;
  u_int8_t flags;
} TBouncerHeader;


/* 
 * TBouncerCmdHello
 *
 * TAG->LOCK
 *
 * used to announce tag to lock
 *
 * hdr.version = BOUNCERPKT_VERSION
 * hdr.command = BOUNCERPKT_CMD_HELLO
 * hdr.value   = amount of retries of hello packet
 * hdr.flags   = RFU
 * salt_a      = first part of 64 bit tag generated salt
 *
 */
typedef struct {  
  TBouncerHeader hdr;
  u_int32_t salt_a;
} TBouncerCmdHello;

/* 
 * TBouncerCmdChallengeSetup
 *
 * LOCK->TAG
 *
 * send information about upcoming challenge to tag
 *
 * hdr.version = BOUNCERPKT_VERSION
 * hdr.command = BOUNCERPKT_CMD_CHALLENGE_SETUP
 * hdr.value   = RFU
 * hdr.flags   = RFU
 * src_mac     = mac address of lock
 * challenge   = 64 bit challenge for lock
 *
 */
typedef struct {  
  TBouncerHeader hdr;
  u_int32_t src_mac;
  u_int32_t challenge[2];
} TBouncerCmdChallengeSetup;

/* 
 * TBouncerCmdChallenge
 *
 * LOCK->TAG
 *
 * send actual challenge to tag
 *
 * hdr.version = BOUNCERPKT_VERSION
 * hdr.command = BOUNCERPKT_CMD_CHALLENGE
 * hdr.value   = BOUNCERPKT_PICKS_LIST_SIZE
 * hdr.flags   = RFU
 * src_mac     = mac address of lock
 * picks       = list of BOUNCERPKT_PICKS_COUNT offsets
 *               out of BOUNCERPKT_PICKS_LIST_SIZE sized
 *               byte array out of calculated tag response
 *
 */
typedef struct {  
  TBouncerHeader hdr;
  u_int32_t src_mac;
  u_int8_t picks[BOUNCERPKT_PICKS_COUNT];
} TBouncerCmdChallenge;

/* 
 * TBouncerCmdResponse
 *
 * TAG->LOCK
 *
 * tag response to lock challenge
 *
 * hdr.version = BOUNCERPKT_VERSION
 * hdr.command = BOUNCERPKT_CMD_RESPONSE
 * hdr.value   = BOUNCERPKT_PICKS_LIST_SIZE
 * hdr.flags   = RFU
 * salt_b      = second part of 64 bit tag generated salt
 * picks       = list of BOUNCERPKT_PICKS_COUNT bytes
 *               out of BOUNCERPKT_PICKS_LIST_SIZE sized
 *               byte array of calculated tag response
 *               that were requested by BOUNCERPKT_CMD_CHALLENGE
 *
 */
typedef struct {  
  TBouncerHeader hdr;
  u_int32_t salt_b;
  u_int8_t picks[BOUNCERPKT_PICKS_COUNT];
} TBouncerCmdResponse;

typedef union
{
  TBouncerHeader pkt_hdr;
  TBouncerChallengeSetup pkt_challenge_setup;
}
TBouncerEnvelope;

#endif/*__OPENBOUNCER_H__*/
