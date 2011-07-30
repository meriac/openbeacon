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

extern unsigned char nRFAPI_Init(unsigned char device,unsigned char rf_channel,const unsigned char *mac,unsigned char mac_size, unsigned char features);
extern void nRFAPI_SetTxPower(unsigned char device,unsigned char power);
extern void nRFAPI_TxRetries(unsigned char device,unsigned char count);
extern void nRFAPI_SetRxMode(unsigned char device,unsigned char receive);
extern void nRFAPI_DynpdEnable(unsigned char device,unsigned char receive);
extern void nRFAPI_PipesEnable(unsigned char device,unsigned char mask);
extern void nRFAPI_PipesAck(unsigned char device,unsigned char mask);
extern unsigned char nRFAPI_GetSizeMac(unsigned char device);
extern unsigned char nRFAPI_SetSizeMac(unsigned char device,unsigned char addr_size);
extern void nRFAPI_GetTxMAC(unsigned char device,unsigned char *addr,unsigned char addr_size);
extern void nRFAPI_SetTxMAC(unsigned char device,const unsigned char *addr,unsigned char addr_size);
extern void nRFAPI_SetRxMAC(unsigned char device,const unsigned char *addr,unsigned char addr_size,unsigned char pipe);
extern void nRFAPI_SetChannel(unsigned char device,unsigned char channel);
extern unsigned char nRFAPI_GetChannel(unsigned char device);
extern unsigned char nRFAPI_ClearIRQ(unsigned char device,unsigned char status);
extern void nRFAPI_TX(unsigned char device,unsigned char *buf,unsigned char count);
extern unsigned char nRFAPI_GetStatus(unsigned char device);
extern unsigned char nRFAPI_GetPipeSizeRX(unsigned char device,unsigned char pipe);
extern void nRFAPI_SetPipeSizeRX(unsigned char device,unsigned char pipe,unsigned char size);
extern unsigned char nRFAPI_GetPipeCurrent(unsigned char device);
extern unsigned char nRFAPI_RX(unsigned char device,unsigned char *buf,unsigned char count);
extern void nRFAPI_FlushRX(unsigned char device);
extern void nRFAPI_FlushTX(unsigned char device);
extern void nRFAPI_ReuseTX(unsigned char device);
extern unsigned char nRFAPI_GetFifoStatus(unsigned char device);
extern unsigned char nRFAPI_CarrierDetect(unsigned char device);
extern void nRFAPI_SetFeatures(unsigned char device,unsigned char features);

#endif/*NRF_API_H*/
