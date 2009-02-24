/***************************************************************
 *
 * OpenBeacon.org - midlevel access functions for
 * issuing raw commands to the nRF24L01 2.4Ghz frontend
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

#ifdef  CE_PIN

#define NRF_SPI_PINS (CSN_PIN|MOSI_PIN|SCK_PIN|MISO_PIN)

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

void nRFCMD_CE(unsigned char enable)
{
    if(enable)
	AT91F_PIO_SetOutput(AT91C_BASE_PIOA,CE_PIN);
    else
	AT91F_PIO_ClearOutput(AT91C_BASE_PIOA,CE_PIN);
}

unsigned char nRFCMD_ReadWriteBuffer(const unsigned char *tx_data, unsigned char *rx_data, 
					unsigned int len)
{
    AT91F_SPI_ReceiveFrame(AT91C_BASE_SPI, rx_data, len, NULL, 0);
    AT91F_SPI_SendFrame(AT91C_BASE_SPI, tx_data, len, NULL, 0);

    AT91C_BASE_PDC_SPI->PDC_PTCR = AT91C_PDC_RXTEN|AT91C_PDC_TXTEN;

    return xSemaphoreTake(xnRF_SemaphoreDMA,100)!=pdTRUE;
} 

static unsigned char nRFCMD_ReadWriteByte(unsigned char reg)
{
    u_int8_t res;
    
    nRFCMD_ReadWriteBuffer(&reg, &res, 1);

    return res;
}

unsigned char nRFCMD_CmdExec(unsigned char cmd)
{
    unsigned char res;
	
    res=nRFCMD_ReadWriteByte(cmd);
		
    return res;
}

unsigned char nRFCMD_RegRead(unsigned char reg)
{
    spi_outbuf[0] = reg;
    spi_outbuf[1] = 0;
	
    nRFCMD_ReadWriteBuffer(spi_outbuf, spi_inbuf, 2);
	
    return spi_inbuf[1];
}

unsigned char nRFCMD_RegWriteStatusRead(unsigned char reg, unsigned char value)
{
    spi_outbuf[0] = reg;
    spi_outbuf[1] = value;
	
    nRFCMD_ReadWriteBuffer(spi_outbuf, spi_inbuf, 2);

    return spi_outbuf[0];
}

unsigned char nRFCMD_RegWriteBuf(unsigned char reg,const unsigned char *buf,unsigned char count)
{
    spi_outbuf[0] = reg;
    memcpy(spi_outbuf+1, buf, count);
    nRFCMD_ReadWriteBuffer(spi_outbuf, spi_inbuf, count+1);
	
    return spi_inbuf[0];
}

unsigned char nRFCMD_RegReadBuf(unsigned char reg,unsigned char *buf,unsigned char count)
{
    spi_outbuf[0] = reg;
    nRFCMD_ReadWriteBuffer(spi_outbuf, spi_inbuf, count+2);
    memcpy(buf, spi_inbuf+1, count);
	
    return spi_inbuf[0];
}

unsigned char nRFCMD_GetRegSize(unsigned char reg)
{
    unsigned char res;
	
    if(reg>0x17)
	res=0;
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
        return pdTRUE;
    }

    AT91F_SPI_SendFrame(AT91C_BASE_SPI, nRFCMD_Macro+1, size, NULL, 0);
    nRFCMD_Macro+=size+1;
    AT91C_BASE_PDC_SPI->PDC_PTCR = AT91C_PDC_TXTEN;
    return pdFALSE;
}

void nRFCMD_ISR_DMA_Handler(void)
{
    portBASE_TYPE xTaskWokenDMA = pdFALSE;    
    
    if((AT91C_BASE_SPI->SPI_SR & AT91C_SPI_ENDTX) && (nRFCMD_ProcessNextMacro()))
	xTaskWokenDMA = xSemaphoreGiveFromISR( xnRF_SemaphoreDMA, &xTaskWokenDMA );
    
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

void nRFCMD_ExecMacro(const unsigned char *macro, unsigned char *rx_data)
{
    nRFCMD_Macro=macro;
    nRFCMD_MacroResult=rx_data;
    nRFCMD_ProcessNextMacro();
    xSemaphoreTake(xnRF_SemaphoreDMA,portMAX_DELAY);
}

void nRFCMD_ISR_ACK_Handler(void)
{
    portBASE_TYPE xTaskWokenACK = pdFALSE;

    if( (AT91C_BASE_PIOA->PIO_ISR & IRQ_PIN) && ((AT91F_PIO_GetInput(AT91C_BASE_PIOA)&IRQ_PIN)==0) )
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

unsigned char nRFCMD_Init(void)
{
    volatile int dummy;
    const int SCBR = ((int)(MCK / 8e6) + 1)&0xFF;
    
    nRFCMD_Macro=nRFCMD_MacroResult=NULL;

    vSemaphoreCreateBinary(xnRF_SemaphoreDMA);
    vSemaphoreCreateBinary(xnRF_SemaphoreACK);
    if(!(xnRF_SemaphoreDMA && xnRF_SemaphoreACK))
	return 1;

    xSemaphoreTake(xnRF_SemaphoreDMA,0);
    xSemaphoreTake(xnRF_SemaphoreACK,0);

    AT91F_SPI_CfgPMC();
    AT91F_PIO_CfgPeriph(AT91C_BASE_PIOA, NRF_SPI_PINS_PERIPHERAL_A, NRF_SPI_PINS_PERIPHERAL_B);
    AT91F_PIO_CfgInput(AT91C_BASE_PIOA, IRQ_PIN);
    AT91F_PIO_ClearOutput(AT91C_BASE_PIOA, CE_PIN);
    AT91F_PIO_CfgOutput(AT91C_BASE_PIOA, CE_PIN);  

    portENTER_CRITICAL();
    
    /* CPOL = 0, NCPHA = 1, CSAAT = 0, BITS = 0000, SCBR = <8MHz, 
     * DLYBS = 0, DLYBCT = 0 */
    AT91C_BASE_SPI->SPI_MR = AT91C_SPI_MSTR | AT91C_SPI_PS_FIXED;
    AT91F_SPI_CfgCs(AT91C_BASE_SPI, 0, AT91C_SPI_BITS_8|AT91C_SPI_NCPHA|(SCBR<<8));
    AT91F_SPI_Enable(AT91C_BASE_SPI);

    AT91F_AIC_ConfigureIt(AT91C_ID_SPI, 4, AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE, nRFCMD_ISR_DMA );
    AT91C_BASE_SPI->SPI_IER = AT91C_SPI_ENDTX;

    /* Enable PIO interrupt for IRQ pin */
    AT91F_AIC_ConfigureIt(AT91C_ID_PIOA, 3, AT91C_AIC_SRCTYPE_HIGH_LEVEL, nRFCMD_ISR_ACK );    
    /* reset IRQ status */
    dummy = AT91C_BASE_PIOA->PIO_ISR;
    AT91C_BASE_PIOA->PIO_IER = IRQ_PIN;
    
    AT91C_BASE_AIC->AIC_IECR = (0x1 << AT91C_ID_SPI) | (0x1 << AT91C_ID_PIOA) ;
    
    portEXIT_CRITICAL();
    
    return 0;
}

#endif /* CE_PIN */
