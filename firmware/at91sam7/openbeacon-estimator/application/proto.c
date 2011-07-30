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
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <string.h>
#include <board.h>
#include <beacontypes.h>
#include <sort.h>
#include "led.h"
#include "xxtea.h"
#include "proto.h"
#include "env.h"
#include "cmd.h"
#include "nRF24L01/nRF_HW.h"
#include "nRF24L01/nRF_CMD.h"
#include "nRF24L01/nRF_API.h"


#define TAG_ID_MASK (0x0000FFFFL)
#define UPDATE_INTERVAL_MS (portTICK_RATE_MS*250)
#define ANNOUNCEMENT_TX_POWER 3
#define SORT_PRINT_DEPTH 8

#define BCFLAGS_VALIDENTRY 0x01

typedef struct {
    u_int8_t bcflags;
    u_int8_t tag_strength;
    u_int16_t reserved;
    u_int16_t tag_oid;        
    u_int32_t arrival_time;
} __attribute__((packed)) TBeaconCache;

// set broadcast mac
const unsigned char broadcast_mac[NRF_MAX_MAC_SIZE]={1,2,3,2,1};
TBeaconCache g_BeaconCache[FIFO_DEPTH];
int g_BeaconSortSize;
TBeaconSort g_BeaconSort[FIFO_DEPTH];
TBeaconSort g_BeaconSortPrint[SORT_PRINT_DEPTH];
unsigned int g_BeaconSortTmp[FIFO_DEPTH];
unsigned int g_BeaconFifoLifeTime;
volatile u_int32_t g_BeaconCacheHead;
TBeaconEnvelope g_Beacon;
xSemaphoreHandle PtSemaphore;

/**********************************************************************/
#define SHUFFLE(a,b)    tmp=g_Beacon.datab[a];\
                        g_Beacon.datab[a]=g_Beacon.datab[b];\
                        g_Beacon.datab[b]=tmp;

/**********************************************************************/
void RAMFUNC shuffle_tx_byteorder (void)
{
    unsigned char tmp;

    SHUFFLE (0 +  0, 3 +  0);
    SHUFFLE (1 +  0, 2 +  0);
    SHUFFLE (0 +  4, 3 +  4);
    SHUFFLE (1 +  4, 2 +  4);
    SHUFFLE (0 +  8, 3 +  8);
    SHUFFLE (1 +  8, 2 +  8);
    SHUFFLE (0 + 12, 3 + 12);
    SHUFFLE (1 + 12, 2 + 12);
}

static inline s_int8_t PtInitNRF(void)
{
    if(!nRFAPI_Init(DEFAULT_DEV, DEFAULT_CHANNEL,broadcast_mac,sizeof(broadcast_mac),ENABLED_NRF_FEATURES))
	return 0;
	
    nRFAPI_SetPipeSizeRX(DEFAULT_DEV, 0,16);
    nRFAPI_SetTxPower(DEFAULT_DEV, ANNOUNCEMENT_TX_POWER);
    nRFAPI_SetRxMode(DEFAULT_DEV, 1);

    nRFCMD_CE(DEFAULT_DEV, 1);

    return 1;
}

static inline unsigned short swapshort (unsigned short src)
{
    return (src>>8)|(src<<8);
}

static inline unsigned long swaplong (unsigned long src)
{
    return (src>>24)|(src<<24)|((src>>8)&0x0000FF00)|((src<<8)&0x00FF0000);
}

void RAMFUNC vnRFUpdateRating(void)
{
    int i,j,count;
    u_int32_t ticks,oid,nid;
    unsigned int *bs,*bss,t;
    TBeaconCache *pcache;
    TBeaconSort *sorted;

    bs=g_BeaconSortTmp;
    pcache=g_BeaconCache;
    ticks=xTaskGetTickCount();

    count=0;
    for(i=0;i<FIFO_DEPTH;i++,pcache++)
	if(pcache->bcflags & BCFLAGS_VALIDENTRY)
	{
	    if((ticks-pcache->arrival_time)>=g_BeaconFifoLifeTime)
		pcache->bcflags=0;
	    else
	    {
		*bs++=(((u_int32_t)pcache->tag_strength)<<24) | pcache->tag_oid;
		count++;
	    }
	}

    if(count>1)
    {
	sort(g_BeaconSortTmp,count);
	bs=bss=g_BeaconSortTmp;
	oid=*bs++;
	
	j=1;
	t=0;
	for(i=0;i<count;i++)
	{
	    nid=*bs++;
	    if((nid!=oid) || (i==count-1))
	    {		
		if(j>0xFF)
		    j=0xFF;		    
		*bss++=oid|((0xFFL-j)<<16);
		t++;
		oid=nid;
		j=1;
	    }
	    else
		j++;
	}

	count=t;
	sort(g_BeaconSortTmp,count);
	
	bs=g_BeaconSortTmp;
	for(i=0;i<(count-1);i++)
	{
	    oid=(*bs++)&TAG_ID_MASK;
	    if(oid)
	    {
		bss=bs;
		for(j=(count-i);j>0;j--)
		{
		    if(((*bss)&TAG_ID_MASK)==oid )
			*bss=0;
		    bss++;
		}
	    }
	}
    }
	
    if( xSemaphoreTake(PtSemaphore, (portTickType)(portTICK_RATE_MS*1000) ) == pdTRUE )
    {
	g_BeaconSortSize=0;
	bs=g_BeaconSortTmp;
	sorted=g_BeaconSort;
	for(i=0;i<count;i++)
	{
	    oid=*bs++;
	    if(oid)
	    {
		sorted->tag_strength=oid>>24;
		sorted->packet_count=0xFF-((u_int8_t)(oid>>16));
		sorted->tag_oid=oid&TAG_ID_MASK;
		sorted++;
		g_BeaconSortSize++;
	    }
	}		
	xSemaphoreGive(PtSemaphore);	
    }	
}

int PtSetFifoLifetimeSeconds(int Seconds)
{
    if((Seconds<1) || (Seconds>30))
	return -1;

    g_BeaconFifoLifeTime=portTICK_RATE_MS*1000*Seconds;
    return Seconds; 
}

int PtGetFifoLifetimeSeconds(void)
{
    return g_BeaconFifoLifeTime/(portTICK_RATE_MS*1000);
}

int vnRFCopyRating(TBeaconSort* Sort,int Items)
{
    if((Items<=0) || (!Sort))
	return 0;
	
    if( xSemaphoreTake(PtSemaphore, (portTickType)(portTICK_RATE_MS*1000) ) == pdTRUE )
    {
	if(Items>g_BeaconSortSize)
	    Items=g_BeaconSortSize;

	memcpy(Sort,g_BeaconSort,sizeof(*Sort)*Items);

	xSemaphoreGive(PtSemaphore);
	return Items;
    }
    else
	return 0;
}

void vnRFtaskRxTx(void *parameter)
{
    TBeaconCache *pcache;
    u_int16_t crc;
    u_int32_t oid;
    portTickType LastUpdateTicks,Ticks;
    (void)parameter;

    if(!PtInitNRF())
	return;

    LastUpdateTicks=xTaskGetTickCount();
	
    for( ; ; )
    {
	if(nRFCMD_WaitRx(10))
        {
	    vLedSetRed(1);

	    do {
		// read packet from nRF chip
		nRFCMD_RegReadBuf(DEFAULT_DEV, RD_RX_PLOAD,g_Beacon.datab,sizeof(g_Beacon));

		// adjust byte order and decode
		shuffle_tx_byteorder();
		xxtea_decode();
		shuffle_tx_byteorder();

		// verify the crc checksum
		crc = env_crc16(g_Beacon.datab, sizeof(g_Beacon)-sizeof(g_Beacon.pkt.crc));
		if(	(swapshort(g_Beacon.pkt.crc)==crc) &&
                	((oid=swaplong(g_Beacon.pkt.oid))<=TAG_ID_MASK)
                        )
		{
		    pcache=&g_BeaconCache[g_BeaconCacheHead];

		    pcache->bcflags=0;
		    pcache->arrival_time=xTaskGetTickCount();
		    pcache->tag_oid=(u_int16_t)oid;
		    pcache->tag_strength=g_Beacon.pkt.strength;
		    pcache->bcflags=BCFLAGS_VALIDENTRY;	

		    g_BeaconCacheHead++;
		    if(g_BeaconCacheHead>=FIFO_DEPTH)
			g_BeaconCacheHead=0;
    		}
	    } while((nRFAPI_GetFifoStatus(DEFAULT_DEV) & FIFO_RX_EMPTY)==0);

	    vLedSetRed(0);
	}
        nRFAPI_ClearIRQ(DEFAULT_DEV, MASK_IRQ_FLAGS);

        // update regularly
        if( ((Ticks=xTaskGetTickCount())-LastUpdateTicks)>UPDATE_INTERVAL_MS )
        {
            LastUpdateTicks=Ticks;
	    vnRFUpdateRating();
        }
    }
}

void vnRFtaskRating(void *parameter)
{
    int count,i;
    (void)parameter;

    for( ; ; )
    {
	vLedSetGreen(1);
	DumpStringToUSB("RX:");

	count=vnRFCopyRating(g_BeaconSortPrint,SORT_PRINT_DEPTH);
	if(count>0)
	{
	    for(i=0;i<count;i++)
	    {
		DumpStringToUSB(" ");
		DumpUIntToUSB(g_BeaconSortPrint[i].tag_strength);
		DumpStringToUSB(",");
		DumpUIntToUSB(g_BeaconSortPrint[i].tag_oid);
		DumpStringToUSB(",");
		DumpUIntToUSB(g_BeaconSortPrint[i].packet_count);
	    }
	}

	DumpStringToUSB("\n\r");
	vLedSetGreen(0);

	vTaskDelay(portTICK_RATE_MS * 950);
    }
}

void vInitProtocolLayer(void)
{
    PtSetFifoLifetimeSeconds(10);

    g_BeaconCacheHead=0;
    memset(&g_Beacon,0,sizeof(g_Beacon));

    PtSemaphore=NULL;
    vSemaphoreCreateBinary( PtSemaphore );

    xTaskCreate (vnRFtaskRxTx, (signed portCHAR *) "nRF_RxTx", TASK_NRF_STACK,
	NULL, TASK_NRF_PRIORITY, NULL);
    xTaskCreate (vnRFtaskRating, (signed portCHAR *) "nRF_Rating", 128,
	NULL, TASK_NRF_PRIORITY, NULL);
}
