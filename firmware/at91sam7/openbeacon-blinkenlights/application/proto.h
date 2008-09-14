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

#define GAMMA_DEFAULT	200
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
    } set_lamp_id;

    struct {
      unsigned char block;
      unsigned short val[8];
    } set_gamma;

    struct {
      unsigned short jitter;
    } set_jitter;

    struct {
      unsigned short emi_pulses;
      unsigned long packet_count;
    } statistics;
  }; /* union */

  unsigned short crc;
} __attribute__ ((packed)) BRFPacket;

extern void vInitProtocolLayer (void);
extern int PtSetFifoLifetimeSeconds (int Seconds);
extern int PtGetFifoLifetimeSeconds (void);

#endif/*__PROTO_H__*/
