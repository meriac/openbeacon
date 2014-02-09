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
extern uint8_t nRFCMD_RegWrite (uint8_t reg,
								const uint8_t * buf, uint8_t count);
extern uint8_t nRFCMD_RegRead (uint8_t reg, uint8_t * buf, uint8_t count);
extern void nRFCMD_Channel (uint8_t channel);
extern void nRFCMD_Config (uint8_t config);
extern void nRFCMD_Macro (const uint8_t * macro);
extern void nRFCMD_Execute (void);
extern void nRFCMD_Listen (uint8_t jiffies);
extern void nRFCMD_ResetStop (void);
extern void nRFCMD_Tx (const uint8_t * buf, uint8_t count, uint8_t jiffies);
extern uint8_t nRFCMD_RegExec (uint8_t reg);
extern uint8_t nRFCMD_RegGet (uint8_t reg);
extern void nRFCMD_RegPut (uint8_t reg, uint8_t value);


#endif /*NRF_CMD_H */
