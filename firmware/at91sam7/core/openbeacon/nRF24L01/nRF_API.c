/***************************************************************
 *
 * OpenBeacon.org - high level nRF24L01+ access functions
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
#include <string.h>
#include <nRF_API.h>
#include <nRF_HW.h>
#include <nRF_CMD.h>
#include <board.h>
#include <beacontypes.h>

#ifndef NRF_RFOPTIONS
#define NRF_RFOPTIONS 0x09
#endif/*NRF_RFOPTIONS*/

// set broadcast MAC to 'BCAST'
const unsigned char rfbroadcast_mac[NRF_MAX_MAC_SIZE]={'T','S','A','C','B'};

static unsigned char nRFAPI_DetectChip(unsigned char device)
{
	unsigned char mac[NRF_MAX_MAC_SIZE],i;
	
	// blank read
	nRFAPI_GetStatus(device);

	// set TX mode
	nRFAPI_SetRxMode (device,0);

	// set dummy MAC size
	nRFAPI_SetSizeMac(device,NRF_MIN_MAC_SIZE);

	// verify dummy MAC size
	if(nRFAPI_GetSizeMac(device)!=NRF_MIN_MAC_SIZE)
		return 0;
	
	// set dummy MAC size
	nRFAPI_SetSizeMac(device,NRF_MAX_MAC_SIZE);
	
	// verify dummy MAC size
	if(nRFAPI_GetSizeMac(device)!=NRF_MAX_MAC_SIZE)
		return 0;
		
	// set dummy MAC
	nRFAPI_SetTxMAC(device,rfbroadcast_mac,NRF_MAX_MAC_SIZE);
	
	//  get dummy MAC
	memset(&mac,0,sizeof(mac));
	nRFAPI_GetTxMAC(device,mac,NRF_MAX_MAC_SIZE);
	
	// if can't verify written MAC - return with error
	for(i=0;i<NRF_MAX_MAC_SIZE;i++)
		if(mac[i]!=rfbroadcast_mac[i])
			return 0;

	// everything is fine
	return 1;
}

void nRFAPI_SetRxMode(unsigned char device,unsigned char receive)
{
	nRFCMD_RegWriteStatusRead(device,CONFIG | WRITE_REG,receive ? 0x3B : 0x3A);
}

unsigned char nRFAPI_Init(
	unsigned char device,
	unsigned char channel,
	const unsigned char *mac,
	unsigned char mac_size,
	unsigned char features
	)
{
	unsigned char i;
	
	// init lower layer
	nRFCMD_Init();
	
	// check validity
	if(	mac_size<3 ||
		mac_size>5 ||
		!nRFAPI_DetectChip(device)
		)
		return 0;
		
	// update mac
	nRFAPI_SetSizeMac(device,mac_size);
	nRFAPI_SetTxMAC(device,mac,mac_size);
	
	// enables pipe
	nRFAPI_SetRxMAC(device,mac,mac_size,0);
	nRFAPI_PipesEnable(device,ERX_P0);
	nRFAPI_PipesAck(device,0);
	nRFAPI_DynpdEnable(device,0);

	// set payload sizes
	for(i=0;i<=5;i++)
		nRFAPI_SetPipeSizeRX(device,i,i?0:mac_size);
	
	// set TX retry count
	nRFAPI_TxRetries(device,0);
	
	// set selected channel
	nRFAPI_SetChannel(device,channel);
	
        // set Tx power
        nRFAPI_SetTxPower(device,3);
	
	// flush FIFOs
	nRFAPI_FlushRX(device);
	nRFAPI_FlushTX(device);

	nRFAPI_SetRxMode(device,0);

	nRFAPI_SetFeatures(device,features);
	return 1;
}

void nRFAPI_SetTxPower(unsigned char device, unsigned char power)
{
    if(power>3)
        power=3;

    nRFCMD_RegWriteStatusRead(device, RF_SETUP | WRITE_REG,NRF_RFOPTIONS | (power<<1));
}

void nRFAPI_TxRetries(unsigned char device, unsigned char count)
{
	if(count>15)
		count=15;

	// setup delay of 250us
	nRFCMD_RegWriteStatusRead (device, SETUP_RETR | WRITE_REG, count);
}

void
nRFAPI_DynpdEnable (unsigned char device, unsigned char mask)
{
  nRFCMD_RegWriteStatusRead (device, DYNPD | WRITE_REG, mask & 0x3F);
}

void nRFAPI_PipesEnable(unsigned char device, unsigned char mask)
{
	nRFCMD_RegWriteStatusRead(device, EN_RXADDR | WRITE_REG,mask & 0x3F);
}

void nRFAPI_PipesAck(unsigned char device, unsigned char mask)
{
	nRFCMD_RegWriteStatusRead(device, EN_AA | WRITE_REG,mask & 0x3F);
}

unsigned char nRFAPI_GetSizeMac(unsigned char device)
{
	unsigned char addr_size;
	
	addr_size=nRFCMD_RegRead(device, SETUP_AW) & 0x03;
	
	return addr_size ? addr_size+2 : 0;
}

unsigned char nRFAPI_SetSizeMac(unsigned char device, unsigned char addr_size)
{
	if(addr_size>=3 && addr_size<=5)
		addr_size-=2;
	else
		addr_size=0;

	nRFCMD_RegWriteStatusRead(device, SETUP_AW | WRITE_REG,addr_size);
	
	return addr_size;
}

void nRFAPI_GetTxMAC(unsigned char device, unsigned char *addr,unsigned char addr_size)
{
	if(addr_size>=3 && addr_size<=5)	
		nRFCMD_RegReadBuf(device, TX_ADDR,addr,addr_size);
}

void nRFAPI_SetTxMAC(unsigned char device, const unsigned char *addr,unsigned char addr_size)
{
	if(addr_size>=3 && addr_size<=5)	
		nRFCMD_RegWriteBuf(device, TX_ADDR | WRITE_REG,addr,addr_size);
}

void nRFAPI_SetRxMAC(unsigned char device, const unsigned char *addr,unsigned char addr_size,unsigned char pipe)
{
	if((pipe<=1 && addr_size>=3 && addr_size<=5)||(addr_size==1 && pipe>=2 && pipe<=5))
		nRFCMD_RegWriteBuf(device, (RX_ADDR_P0 + pipe) | WRITE_REG,addr,addr_size);
}

void nRFAPI_SetChannel(unsigned char device, unsigned char channel)
{
	nRFCMD_RegWriteStatusRead(device, RF_CH | WRITE_REG,channel&0x7f);
}

unsigned char nRFAPI_GetChannel(unsigned char device)
{
	return nRFCMD_RegRead(device, RF_CH) & 0x7F;
}

unsigned char nRFAPI_ClearIRQ(unsigned char device, unsigned char status)
{
	return nRFCMD_RegWriteStatusRead(device, STATUS | WRITE_REG, status & MASK_IRQ_FLAGS);
}

void nRFAPI_TX(unsigned char device, unsigned char *buf,unsigned char count)
{
	nRFCMD_RegWriteBuf(device, WR_TX_PLOAD,buf,count);
}

unsigned char nRFAPI_GetStatus(unsigned char device)
{
	return nRFCMD_CmdExec(device, OP_NOP);
}

unsigned char nRFAPI_GetPipeSizeRX(unsigned char device, unsigned char pipe)
{
	if(pipe<=5)
		return nRFCMD_RegRead(device, RX_PW_P0 + pipe);
	else
		return 0;		
}

void nRFAPI_SetPipeSizeRX(unsigned char device, unsigned char pipe,unsigned char size)
{
	if(pipe<=5)
		nRFCMD_RegWriteStatusRead(device, (RX_PW_P0 + pipe) | WRITE_REG,size);
}

unsigned char nRFAPI_GetPipeCurrent(unsigned char device)
{
	return (nRFAPI_GetStatus(device)>>1) & 0x7;
}

unsigned char nRFAPI_RX(unsigned char device, unsigned char *buf,unsigned char count)
{
	unsigned char size,pipe;
	
	pipe=nRFAPI_GetPipeCurrent(device);
	if(pipe>=7)
		size=0;
	else
	{	
		size=nRFAPI_GetPipeSizeRX(device, pipe);
	
		if(size<=count)
			nRFCMD_RegReadBuf(device, RD_RX_PLOAD,buf,size);
		else
		{		
			nRFAPI_FlushRX(device);
			size=0;
		}
	}
	
	return size;	
}

void nRFAPI_FlushRX(unsigned char device)
{
	nRFCMD_CmdExec(device,FLUSH_RX);
}

void nRFAPI_FlushTX(unsigned char device)
{
	nRFCMD_CmdExec(device,FLUSH_TX);
}

void nRFAPI_ReuseTX(unsigned char device)
{
	nRFCMD_CmdExec(device,REUSE_TX_PL);
}

unsigned char nRFAPI_GetFifoStatus(unsigned char device)
{
	return nRFCMD_RegRead(device,FIFO_STATUS);
}

unsigned char nRFAPI_CarrierDetect(unsigned char device)
{
	return nRFCMD_RegRead(device,CD);
}

void nRFAPI_SetFeatures(unsigned char device, unsigned char features)
{
#ifdef  __NRF24L01__
	unsigned const char ACTIVATE_SEQUENCE[] = {ACTIVATE, 0x73};
	unsigned char dummy_buffer[sizeof(ACTIVATE_SEQUENCE)] = {0,0};
	nRFCMD_ReadWriteBuffer(device,ACTIVATE_SEQUENCE, dummy_buffer, sizeof(ACTIVATE_SEQUENCE));
#endif/*__NRF24L01__*/
	nRFCMD_RegWriteStatusRead(device,FEATURE, features);
}
