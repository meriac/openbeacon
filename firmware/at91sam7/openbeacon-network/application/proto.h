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

#define FIFO_DEPTH	256
#define RF_PAYLOAD_SIZE	26

enum {
  RF_CMD_SET_VALUES,
  RF_CMD_SET_LAMP_ID,
  RF_CMD_SET_GAMMA,
  RF_CMD_WRITE_CONFIG,
  RF_CMD_SET_JITTER,
  RF_CMD_SEND_STATISTICS,
  RF_CMD_ENTER_UPDATE_MODE = 0x3f
};

typedef struct
{
  unsigned char cmd;
  unsigned short mac;
  unsigned char wmcu_id;

  union {
    unsigned char payload[RF_PAYLOAD_SIZE];
    
    struct {
      unsigned char id;
      unsigned char wmcu_id;
    } PACKED set_lamp_id;

    struct {
      unsigned char block;
      unsigned short val[8];
    } PACKED set_gamma;

    struct {
      unsigned short jitter;
    } PACKED set_jitter;

    struct {
      unsigned short emi_pulses;
      unsigned long packet_count;
    } PACKED statistics;

  } PACKED; /* union */

  unsigned short crc;
} PACKED BRFPacket;

static inline unsigned short
swapshort (unsigned short src)
{
  return (src >> 8) | (src << 8);
}

static inline unsigned long
swaplong (unsigned long src)
{
  return (src >> 24) |
  	 (src << 24) | 
	 ((src >> 8) & 0x0000FF00) |
	 ((src << 8) & 0x00FF0000);
}

extern void vInitProtocolLayer (void);
extern void vnRFTransmitPacket(BRFPacket *pkg);
extern int PtSetFifoLifetimeSeconds (int Seconds);
extern int PtGetFifoLifetimeSeconds (void);
extern void PtDumpUIntToUSB (unsigned int data);
extern void PtDumpStringToUSB (const char *text);
void shuffle_tx_byteorder (unsigned long *v, int len);

#endif/*__PROTO_H__*/
