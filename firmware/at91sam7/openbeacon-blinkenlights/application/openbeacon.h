/***************************************************************
 *
 * OpenBeacon.org - OnAir protocol specification and definition
 *
 * Copyright 2006 Milosch Meriac <meriac@openbeacon.de>
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

#ifndef __OPENBEACON_H__
#define __OPENBEACON_H__

#define RFBPROTO_BLINKENLIGHTS	42
#define RFB_RFOPTIONS		0x0F
#define ENABLED_NRF_FEATURES	0x00

#define RF_PKG_SENT_BY_DIMMER	0x40
#define RF_PKG_REPLY_WANTED	0x80

#define PACKED __attribute__ ((packed))

#define GAMMA_DEFAULT	200
#define FIFO_DEPTH	256
#define RF_PAYLOAD_SIZE	16

enum
{
  RF_CMD_SET_VALUES,
  RF_CMD_SET_LAMP_ID,
  RF_CMD_SET_GAMMA,
  RF_CMD_WRITE_CONFIG,
  RF_CMD_SET_JITTER,
  RF_CMD_SEND_STATISTICS,
  RF_CMD_SET_DIMMER_DELAY,
  RF_CMD_SET_DIMMER_CONTROL,
  RF_CMD_PING,
  RF_CMD_RESET,
  RF_CMD_ENTER_UPDATE_MODE = 0x3f
};

typedef struct
{
  unsigned short mac;
  unsigned char wmcu_id;
  unsigned char cmd;

  union
  {
    unsigned char payload[RF_PAYLOAD_SIZE];
    unsigned char dummy[6];

    struct
    {
      unsigned char id;
      unsigned char wmcu_id;
    } PACKED set_lamp_id;

    struct
    {
      unsigned char block;
      unsigned short val[8];
    } PACKED set_gamma;

    struct
    {
      unsigned short jitter;
    } PACKED set_jitter;

    struct
    {
      unsigned short emi_pulses;
      unsigned long packet_count;
      unsigned long pings_lost;
      unsigned long fw_version;
      unsigned long tick_count;
    } PACKED statistics;

    struct
    {
      unsigned short delay;
    } PACKED set_delay;

    struct
    {
      unsigned char off;
    } PACKED dimmer_control;

    struct
    {
      unsigned int sequence;
    } PACKED ping;

  } PACKED;			/* union */

  unsigned int sequence;
  unsigned int crc;
} PACKED BRFPacket;

#endif/*__OPENBEACON_H__*/
