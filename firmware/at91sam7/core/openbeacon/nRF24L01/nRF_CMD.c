/***************************************************************
 *
 * OpenBeacon.org - midlevel access functions for
 * issuing raw commands to the nRF24L01+ 2.4Ghz frontend
 *
 * Copyright 2007 Milosch Meriac <meriac@openbeacon.de>
 *
 * provides generic register level access functions
 * for accessing nRF24L01 registers at a generic level
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
#include <AT91SAM7.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <string.h>
#include <task.h>
#include <nRF_HW.h>
#include <nRF_CMD.h>
#include <board.h>
#include <beacontypes.h>

#ifdef  CSN_ID
#define OB0_CSN_ID CSN_ID
#endif/*CSN_ID*/

#ifdef  CSN_PIN
#define OB0_CSN_PIN CSN_PIN
#endif/*CSN_PIN*/

#ifdef  MISO_PIN
#define OB0_MISO_PIN MISO_PIN
#endif/*MISO_PIN*/

#ifdef  MOSI_PIN
#define OB0_MOSI_PIN MOSI_PIN
#endif/*MOSI_PIN*/

#ifdef  SCK_PIN
#define OB0_SCK_PIN SCK_PIN
#endif/*SCK_PIN*/

#ifdef  IRQ_PIN
#define OB0_IRQ_PIN IRQ_PIN
#endif/*IRQ_PIN*/

#ifdef  CE_PIN
#define OB0_CE_PIN CE_PIN
#endif/*CE_PIN*/

#ifdef  OB1_IRQ_PIN
#define OB_IRQ_PINS (OB0_IRQ_PIN|OB1_IRQ_PIN)
#else
#define OB_IRQ_PINS (OB0_IRQ_PIN)
#endif/*OB1_IRQ_PIN*/

#ifdef  OB1_CE_PIN
#define OB_CE_PINS (OB0_CE_PIN|OB1_CE_PIN)
#else
#define OB_CE_PINS (OB0_CE_PIN)
#endif/*OB1_CE_PIN*/

#ifdef  OB1_CSN_PIN
#define OB_CSN_PINS (OB0_CSN_PIN|OB1_CSN_PIN)
#else
#define OB_CSN_PINS (OB0_CSN_PIN)
#endif/*OB1_CSN_PIN*/

#ifdef  OB0_CE_PIN

#ifndef OB0_CSN_ID
#define OB0_CSN_ID 0
#endif/*OB0_CSN_ID*/

/* CPOL = 0, NCPHA = 1, CSAAT = 0, BITS = 0000, SCBR = <8MHz,
 * DLYBS = 0, DLYBCT = 0 */
#define NRF_SPI_SETTINGS (AT91C_SPI_BITS_8|AT91C_SPI_NCPHA|(SCBR<<8))
#define NRF_SPI_MR (AT91C_SPI_MSTR | AT91C_SPI_PS_FIXED | AT91C_SPI_PCS)

#ifndef CSN_PIN_PIO
#ifdef  OB1_CSN_PIN
#define NRF_SPI_PINS (OB0_CSN_PIN|OB1_CSN_PIN|MOSI_PIN|SCK_PIN|MISO_PIN)
#else
#define NRF_SPI_PINS (OB0_CSN_PIN|MOSI_PIN|SCK_PIN|MISO_PIN)
#endif/*OB1_CSN_PIN*/
#else
#define NRF_SPI_PINS (MOSI_PIN|SCK_PIN|MISO_PIN)
#endif/*CSN_PIN_PIO*/

#ifdef  __AT91SAM7X256__

#define AT91C_BASE_SPI AT91C_BASE_SPI1
#define AT91C_BASE_PDC_SPI AT91C_BASE_PDC_SPI1
#define AT91C_ID_SPI AT91C_ID_SPI1
#define AT91F_SPI_CfgPMC AT91F_SPI1_CfgPMC
#define NRF_SPI_PINS_PERIPHERAL_A 0
#define NRF_SPI_PINS_PERIPHERAL_B NRF_SPI_PINS

#else /*if not __AT91SAM7X256__*/

#define NRF_SPI_PINS_PERIPHERAL_A NRF_SPI_PINS
#define NRF_SPI_PINS_PERIPHERAL_B 0

#endif/*__AT91SAM7X256__*/

#define SPI_MAX_XFER_LEN 33
#define NRFCMD_MACRO_READ 0x80

static u_int8_t spi_outbuf[SPI_MAX_XFER_LEN];
static u_int8_t spi_inbuf[SPI_MAX_XFER_LEN];
static xSemaphoreHandle xnRF_SemaphoreDMA;
static xSemaphoreHandle xnRF_SemaphoreACK;

const unsigned char *nRFCMD_Macro;
unsigned char *nRFCMD_MacroResult;

void nRFCMD_CE(
	unsigned char device,
	unsigned char enable)
{
#ifdef  OB1_CE_PIN
    if(device)
    {
	if(enable)
	  AT91F_PIO_SetOutput(AT91C_BASE_PIOA,OB1_CE_PIN);
	else
	  AT91F_PIO_ClearOutput(AT91C_BASE_PIOA,OB1_CE_PIN);
    }
    else
#else /*OB1_CE_PIN*/
    if(!device)
#endif/*OB1_CE_PIN*/
    {
	if(enable)
	  AT91F_PIO_SetOutput(AT91C_BASE_PIOA,OB0_CE_PIN);
	else
	  AT91F_PIO_ClearOutput(AT91C_BASE_PIOA,OB0_CE_PIN);
    }
}

static void nRFCMD_UpdateDevice(
	unsigned char device)
{
    AT91C_BASE_SPI->SPI_MR = ( device ?
#ifdef  OB1_CSN_ID
	NRF_SPI_MR ^ (1<<(16+OB1_CSN_ID)) :
#else
	0 :
#endif/*OB1_CSN_ID*/
	NRF_SPI_MR ^ (1<<(16+OB0_CSN_ID))
	);
}

unsigned char nRFCMD_ReadWriteBuffer(
	unsigned char device,
	const unsigned char *tx_data,
	unsigned char *rx_data,
	unsigned int len)
{
    nRFCMD_UpdateDevice(device);

    AT91F_SPI_ReceiveFrame(AT91C_BASE_SPI, rx_data, len, NULL, 0);
    AT91F_SPI_SendFrame(AT91C_BASE_SPI, tx_data, len, NULL, 0);

    AT91C_BASE_PDC_SPI->PDC_PTCR = AT91C_PDC_RXTEN|AT91C_PDC_TXTEN;

    return xSemaphoreTake(xnRF_SemaphoreDMA,100)!=pdTRUE;
}

static unsigned char nRFCMD_ReadWriteByte(
	unsigned char device,
	unsigned char reg)
{
    u_int8_t res;

    nRFCMD_ReadWriteBuffer(device, &reg, &res, 1);

    return res;
}

unsigned char nRFCMD_CmdExec(unsigned char device, unsigned char cmd)
{
    return nRFCMD_ReadWriteByte(device, cmd);
}

unsigned char nRFCMD_RegRead(
	unsigned char device,
	unsigned char reg)
{
    spi_outbuf[0] = reg;
    spi_outbuf[1] = 0;

    nRFCMD_ReadWriteBuffer(device, spi_outbuf, spi_inbuf, 2);

    return spi_inbuf[1];
}

unsigned char nRFCMD_RegWriteStatusRead(
	unsigned char device,
	unsigned char reg,
	unsigned char value)
{
    spi_outbuf[0] = reg;
    spi_outbuf[1] = value;
	
    nRFCMD_ReadWriteBuffer(device, spi_outbuf, spi_inbuf, 2);

    return spi_outbuf[0];
}

unsigned char nRFCMD_RegWriteBuf(
	unsigned char device,
	unsigned char reg,
	const unsigned char *buf,
	unsigned char count)
{
    spi_outbuf[0] = reg;
    memcpy(spi_outbuf+1, buf, count);
    nRFCMD_ReadWriteBuffer(device, spi_outbuf, spi_inbuf, count+1);

    return spi_inbuf[0];
}

unsigned char nRFCMD_RegReadBuf(
	unsigned char device,
	unsigned char reg,
	unsigned char *buf,
	unsigned char count)
{
    spi_outbuf[0] = reg;
    nRFCMD_ReadWriteBuffer(device, spi_outbuf, spi_inbuf, count+2);
    memcpy(buf, spi_inbuf+1, count);

    return spi_inbuf[0];
}

unsigned char nRFCMD_GetRegSize(unsigned char reg)
{
    unsigned char res;
	
    if(reg>0x17)
	res=(reg==0x1C)?1:0;
    else
	switch(reg)
	{
	    case RX_ADDR_P0:
	    case RX_ADDR_P1:
	    case TX_ADDR:
		res=NRF_MAX_MAC_SIZE;
		break;
	    default:
	    	res=1;
	}
    return res;
}

static portBASE_TYPE nRFCMD_ProcessNextMacro(void)
{
    u_int8_t size;

    if(!nRFCMD_Macro)
        return pdTRUE;

    if((size=*nRFCMD_Macro)==0)
    {
        nRFCMD_Macro=NULL;
        return pdFALSE;
    }

    AT91F_SPI_SendFrame(AT91C_BASE_SPI, nRFCMD_Macro+1, size, NULL, 0);
    nRFCMD_Macro+=size+1;
    AT91C_BASE_PDC_SPI->PDC_PTCR = AT91C_PDC_TXTEN;
    return pdTRUE;
}

void nRFCMD_ISR_DMA_Handler(void)
{
    portBASE_TYPE xTaskWokenDMA = pdFALSE;

    if(	(AT91C_BASE_SPI->SPI_SR & AT91C_SPI_ENDTX)
	&& nRFCMD_ProcessNextMacro()
	)
    {
	xTaskWokenDMA = xSemaphoreGiveFromISR( xnRF_SemaphoreDMA, &xTaskWokenDMA );
    }

    AT91C_BASE_AIC->AIC_EOICR = 0;

    if(xTaskWokenDMA)
	portYIELD_FROM_ISR();
}

void __attribute__((naked)) nRFCMD_ISR_DMA(void)
{
    portSAVE_CONTEXT();
    nRFCMD_ISR_DMA_Handler();
    portRESTORE_CONTEXT();
}

void nRFCMD_ExecMacro(
	unsigned char device,
	const unsigned char *macro,
	unsigned char *rx_data)
{
    nRFCMD_UpdateDevice(device);

    nRFCMD_Macro=macro;
    nRFCMD_MacroResult=rx_data;
    nRFCMD_ProcessNextMacro();
    xSemaphoreTake(xnRF_SemaphoreDMA,portMAX_DELAY);
}

void nRFCMD_ISR_ACK_Handler(void)
{
    portBASE_TYPE xTaskWokenACK = pdFALSE;

    if( ( AT91C_BASE_PIOA->PIO_ISR & OB_IRQ_PINS ) &&
	((AT91F_PIO_GetInput(AT91C_BASE_PIOA)&OB_IRQ_PINS)!=OB_IRQ_PINS)
	)
	xTaskWokenACK = xSemaphoreGiveFromISR( xnRF_SemaphoreACK, &xTaskWokenACK );

    AT91C_BASE_AIC->AIC_EOICR = 0;

    if(xTaskWokenACK)
	portYIELD_FROM_ISR();
}

void __attribute__((naked)) nRFCMD_ISR_ACK(void)
{
    portSAVE_CONTEXT();
    nRFCMD_ISR_ACK_Handler();
    portRESTORE_CONTEXT();
}

unsigned char nRFCMD_WaitRx(unsigned int ticks)
{
    return xSemaphoreTake(xnRF_SemaphoreACK,ticks);
}

unsigned char nRFCMD_PendingDevice(unsigned char device)
{
#ifndef OB1_IRQ_PIN
    if(device)
	return 0;
#endif/*OB1_IRQ_PIN*/
    return ( AT91F_PIO_GetInput(AT91C_BASE_PIOA)
#ifdef  OB1_IRQ_PIN
	& (device?OB1_IRQ_PIN:OB0_IRQ_PIN) )==0;
#else
	& OB0_IRQ_PIN )==0;
#endif/*OB1_IRQ_PIN*/
}

unsigned char nRFCMD_Init(void)
{
    static int firstrun = 0;
    volatile int dummy __attribute__((unused));
    const int SCBR = ((int)(MCK / 4e6) + 1)&0xFF;

    /* make sure that init runs only once */
    if(firstrun)
	return 0;
    else
	firstrun=1;

    nRFCMD_Macro=nRFCMD_MacroResult=NULL;

    vSemaphoreCreateBinary(xnRF_SemaphoreDMA);
    vSemaphoreCreateBinary(xnRF_SemaphoreACK);
    if(!(xnRF_SemaphoreDMA && xnRF_SemaphoreACK))
	return 1;

    xSemaphoreTake(xnRF_SemaphoreDMA,0);
    xSemaphoreTake(xnRF_SemaphoreACK,0);

    AT91F_SPI_CfgPMC();
    AT91F_PIO_CfgPeriph(AT91C_BASE_PIOA, NRF_SPI_PINS_PERIPHERAL_A, NRF_SPI_PINS_PERIPHERAL_B);

#ifdef  CSN_PIN_PIO
    AT91F_PIO_CfgPeriph(CSN_PIN_PIO, OB_CSN_PINS, 0);
#endif/*CSN_PIN_PIO*/

    AT91F_PIO_CfgInput(AT91C_BASE_PIOA, OB_IRQ_PINS);
    AT91F_PIO_CfgOutput(AT91C_BASE_PIOA, OB_CE_PINS);
    AT91F_PIO_ClearOutput(AT91C_BASE_PIOA, OB_CE_PINS);

    portENTER_CRITICAL();

    AT91C_BASE_SPI->SPI_MR = NRF_SPI_MR;
    AT91F_SPI_CfgCs(AT91C_BASE_SPI, OB0_CSN_ID, NRF_SPI_SETTINGS);
#ifdef OB1_CSN_ID
    AT91F_SPI_CfgCs(AT91C_BASE_SPI, OB1_CSN_ID, NRF_SPI_SETTINGS);
#endif
    AT91F_SPI_Enable(AT91C_BASE_SPI);

    AT91F_AIC_ConfigureIt(AT91C_ID_SPI, 4, AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE, nRFCMD_ISR_DMA );
    AT91C_BASE_SPI->SPI_IER = AT91C_SPI_ENDTX;

    /* Enable PIO interrupt for IRQ pin */
    AT91F_AIC_ConfigureIt(AT91C_ID_PIOA, 3, AT91C_AIC_SRCTYPE_HIGH_LEVEL, nRFCMD_ISR_ACK );
    /* reset IRQ status */
    dummy = AT91C_BASE_PIOA->PIO_ISR;
    AT91C_BASE_PIOA->PIO_IER = OB_IRQ_PINS;

    AT91C_BASE_AIC->AIC_IECR = (0x1 << AT91C_ID_SPI) | (0x1 << AT91C_ID_PIOA) ;

    portEXIT_CRITICAL();

    return 0;
}

#endif /* OB0_CE_PIN */
