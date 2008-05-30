/***************************************************************
 *
 * OpenBeacon.org - function definitions
 *
 * Copyright 2007 Milosch Meriac <meriac@openbeacon.de>
 *
 * provides high level initialization and startup sanity
 * checks and test routines to verify that the chip is working
 * properly and no soldering errors occored on the digital part.
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
#ifndef NRF_API_H
#define NRF_API_H

extern unsigned char nRFAPI_Init(unsigned char channel,const unsigned char *mac,unsigned char mac_size, unsigned char features);
extern void nRFAPI_SetTxPower(unsigned char power);
extern void nRFAPI_TxRetries(unsigned char count);
extern void nRFAPI_SetRxMode(unsigned char receive);
extern void nRFAPI_PipesEnable(unsigned char mask);
extern void nRFAPI_PipesAck(unsigned char mask);
extern unsigned char nRFAPI_GetSizeMac(void);
extern unsigned char nRFAPI_SetSizeMac(unsigned char addr_size);
extern void nRFAPI_GetTxMAC(unsigned char *addr,unsigned char addr_size);
extern void nRFAPI_SetTxMAC(const unsigned char *addr,unsigned char addr_size);
extern void nRFAPI_SetRxMAC(const unsigned char *addr,unsigned char addr_size,unsigned char pipe);
extern void nRFAPI_SetChannel(unsigned char channel);
extern unsigned char nRFAPI_GetChannel(void);
extern unsigned char nRFAPI_ClearIRQ(unsigned char status);
extern void nRFAPI_TX(unsigned char *buf,unsigned char count);
extern unsigned char nRFAPI_GetStatus(void);
extern unsigned char nRFAPI_GetPipeSizeRX(unsigned char pipe);
extern void nRFAPI_SetPipeSizeRX(unsigned char pipe,unsigned char size);
extern unsigned char nRFAPI_GetPipeCurrent(void);
extern unsigned char nRFAPI_RX(unsigned char *buf,unsigned char count);
extern void nRFAPI_FlushRX(void);
extern void nRFAPI_FlushTX(void);
extern void nRFAPI_ReuseTX(void);
extern unsigned char nRFAPI_GetFifoStatus(void);
extern unsigned char nRFAPI_CarrierDetect(void);
extern void nRFAPI_SetFeatures(unsigned char features);

#endif/*NRF_API_H*/
