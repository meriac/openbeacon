/* blib - Library of useful things to hack the Blinkenlights
 *
 * Copyright (c) 2001-2008  The Blinkenlights Crew
 *                          Tim Pritlove <tim@ccc.de>
 *                          Sven Neumann <sven@gimp.org>
 *                          Michael Natterer <mitch@gimp.org>
 *                          Daniel Mack <daniel@yoobay.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __B_PROTOCOL_H__
#define __B_PROTOCOL_H__

#define MAGIC_MCU_SETUP    	0x2342FEED  /* MCU Configuration packet		*/
#define MAGIC_MCU_FRAME     	0x23542666  /* MCU Frame packet			*/
#define MAGIC_MCU_DEVCTRL   	0x23542667  /* MCU Device Control packet	*/
#define MAGIC_MCU_MULTIFRAME	0x23542668  /* MCU MultiFrame packet		*/
#define MAGIC_MCU_RESPONSE	0xFEEDBACC  /* MCU response packet              */
#define MAGIC_WDIM_RESPONSE	0xFEEDBACD  /* WDIM response packet             */

#define MAGIC_BLFRAME		0xDEADBEEF  /* Original BL Frame Packet              */
#define MAGIC_BLFRAME_256	0xFEEDBEEF  /* Extendend BL Frame Packet (Greyscale) */

#define MAGIC_HEARTBEAT		0x42424242  /* Heartbeat packet                      */

#define MCU_LISTENER_PORT	2323
#define MCU_RESPONSE_PORT	23232
#define MCU_ID_ANY		-1

#define B_HEARTBEAT_PORT	4242
#define B_HEARTBEAT_INTERVAL	5000	/* Heartbeat interval in ms    */

/***********************************************************/

/*
 * Legacy Blinkenlights bl_frame format
 */

typedef struct bl_frame_header bl_frame_header_t;

struct bl_frame_header
{
  unsigned int frame_magic;  /* == MAGIC_BLFRAME */
  unsigned int frame_count;
  unsigned short frame_width;
  unsigned short frame_height;
  /*
   * followed by
   * unsigned char data[rows][columns];
   */
} PACKED;

/***********************************************************/

/*
 * MCU Frame packet
 */

typedef struct mcu_frame_header mcu_frame_header_t;

struct mcu_frame_header
{
  unsigned int magic;     /* == MAGIC_MCU_FRAME                        */
  unsigned short height;    /* rows                                      */
  unsigned short width;     /* columns                                   */
  unsigned short channels;  /* Number of channels (mono/grey: 1, rgb: 3) */
  unsigned short bpp;       /* bits used per pixel information (only 4 and 8 supported */
  /*
   * followed by
   * unsigned char data[rows * columns * channels * (8/bpp)];
   */
} PACKED;

/*
 * MCU Multi Frame packet
 *
 *
 * - One Multi Frame packet may contain multiple frames, but does not need to
 * - If the number and ids of the subframes vary in consecutive multi frame packets 
 *   then nothing is assumed about the missing subframes. This allows for incremental
 *   updates for only the screens that did change. 
 *
 */

typedef struct mcu_subframe_header mcu_subframe_header_t;

struct mcu_subframe_header
{
  unsigned char screen_id;		/* screen id                                 */
  unsigned char bpp;			/* bits per pixel, supported values: (4,8)   */
					/* 4 means nibbles 8 means bytes             */
  unsigned short height;		/* number of rows                            */
  unsigned short width;			/* width in pixels of row                    */
  /*
   * followed by 
   * nibbles in [rows][columns];
   * if width is uneven one nibble is used as padding
   * or bytes[]
   *
   * the bytesize of this can be calculated using height * width in the byte case 
   *   and height * ((width + 1)/2) in case of nibbles (integer divison) 
   */
  unsigned char data[0];
} PACKED;

typedef struct mcu_multiframe_header mcu_multiframe_header_t;

struct mcu_multiframe_header
{
  unsigned int magic;				/* == MAGIC_MCU_MULTIFRAME                   */
  unsigned int timestamp_h, timestamp_l;	/* milliseconds since epoch - e.g. gettimeofday(&tv); 
						   timeStamp = tv->tv_sec * 1000 + tv->tv_usec / 1000.; */
  /*
   * followed by multiple subframe headers
   */
  mcu_subframe_header_t subframe[0];
} PACKED;


/*
 * MCU Setup packet
 */

typedef struct mcu_setup_pixel mcu_setup_pixel_t;

struct mcu_setup_pixel
{
  unsigned char row;
  unsigned char column;
} PACKED;


typedef struct mcu_setup_header mcu_setup_header_t;

struct mcu_setup_header
{
  unsigned int     magic;        /* == MAGIC_MCU_SETUP        */

  char          mcu_id;       /* target MCU id ( -1 = any) */
  unsigned char _reserved[3]; /* padding                   */

  unsigned short     height;
  unsigned short     width;
  unsigned short     channels;

  unsigned short     pixels;       /* number of ports used (starting from 0) */
  /*
   * followed by
   * mcu_setup_pixel_t pixel[pixels];
   */
} PACKED;


/*
 * MCU Device Control packet
 */

#define MCU_DEVCTRL_COMMAND_SET_MCU_ID		0	/* set MCU's ID					*/
#define MCU_DEVCTRL_COMMAND_SET_LAMP_ID		1	/* set the ID of a lamp (MAC in *param) 	*/
#define MCU_DEVCTRL_COMMAND_SET_GAMMA		2	/* set the gamma curve of a lamp		*/
#define MCU_DEVCTRL_COMMAND_WRITE_CONFIG	3	/* tell the MCU to write the gamma curve	*/
#define MCU_DEVCTRL_COMMAND_SET_JITTER		4	/* set the jitter for a lamp			*/
#define MCU_DEVCTRL_COMMAND_SET_ASSIGNED_LAMPS	5	/* set lamps assigned to this MCU		*/
#define MCU_DEVCTRL_COMMAND_SEND_WDIM_STATS	6	/* check WDIM statistics			*/
#define MCU_DEVCTRL_COMMAND_SET_DIMMER_DELAY	8	/* set dimmer delay 				*/
#define MCU_DEVCTRL_COMMAND_SET_DIMMER_CONTROL	9	/* set dimmer control (force off) 		*/
#define MCU_DEVCTRL_COMMAND_SET_RF_POWER	10	/* set RF power level				*/
#define MCU_DEVCTRL_COMMAND_SET_JAM_DENSITY	11	/* set JAM density (pkt / ms)			*/
#define MCU_DEVCTRL_COMMAND_SEND_WMCU_STATS	12	/* check WMCU stats				*/
#define MCU_DEVCTRL_COMMAND_RESET_MCU		13	/* reset WMCU 					*/
#define MCU_DEVCTRL_COMMAND_RESET_WDIM		14	/* reset WMCU 					*/
#define MCU_DEVCTRL_COMMAND_OUTPUT_RAW		0xff	/* DEBUG: output raw RF packet			*/

typedef struct mcu_devctrl_header  mcu_devctrl_header_t;

struct mcu_devctrl_header
{
  unsigned int magic;         /* == MAGIC_MCU_DEVCTRL			*/
  unsigned int command;       /* MCU_DEVCTRL_COMMAND_*			*/
  unsigned int mac;           /* LAMP MAC address (if needed)		*/
  unsigned int value;
  /* params consume the rest of the packet, up to MTU */
  unsigned int param[0];
} PACKED;

/*
 * Heartbeat Packet
 */

typedef struct heartbeat_header heartbeat_header_t;

struct heartbeat_header
{
  unsigned int magic;        /* == MAGIC_HEARTBEAT                      */
  unsigned short version;    /* hearbeat protocol version number (0)    */
} PACKED;


/* function prototypes */
void bprotocol_init(void);
void b_parse_rfrx_pkg(BRFPacket *pkg);
void b_set_lamp_id (int lamp_id, int lamp_mac);

/* externals */
extern unsigned int b_rec_total;
extern unsigned int b_rec_frames;
extern unsigned int b_rec_setup;
extern unsigned int sequence_seed;

#endif /* __B_PROTOCOL_H__ */
