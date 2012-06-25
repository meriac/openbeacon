/***************************************************************
 *
 * OpenBeacon.org - definition of exported functions
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

#ifndef NRF_CMD_H
#define NRF_CMD_H

// LNA, RF output -18dBm, 2Mbps
#define NRF_RFOPTIONS 0x09

#define NRF_MAC_SIZE 5

extern void nRFCMD_Init (void);
extern unsigned char nRFCMD_RegWrite (unsigned char reg,
									  const unsigned char *buf,
									  unsigned char count);
extern unsigned char nRFCMD_RegRead (unsigned char reg, unsigned char *buf,
									 unsigned char count);
extern void nRFCMD_Macro (const unsigned char *macro);
extern void nRFCMD_Execute (void);
extern void nRFCMD_Stop (void);
extern void nRFCMD_Start (void);
extern void nRFCMD_StartRX (void);
extern unsigned char nRFCMD_RegGet (unsigned char reg);
extern unsigned char nRFCMD_RegExec (unsigned char reg);
extern unsigned char nRFCMD_RegReadWrite (unsigned char reg,
										  unsigned char value);

#endif /*NRF_CMD_H */
