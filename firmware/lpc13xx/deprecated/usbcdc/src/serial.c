/*----------------------------------------------------------------------------
 *      Name:    serial.c
 *      Purpose: serial port handling for LPC134x
 *      Version: V1.10
 *----------------------------------------------------------------------------
 *      This software is supplied "AS IS" without any warranties, express,
 *      implied or statutory, including but not limited to the implied
 *      warranties of fitness for purpose, satisfactory quality and
 *      noninfringement. Keil extends you a royalty-free right to reproduce
 *      and distribute executable files created using this software for use
 *      on NXP Semiconductors LPC microcontroller devices only. Nothing else 
 *      gives you the right to use this software.
 *
 * Copyright (c) 2009 Keil - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/
#include "LPC13xx.h"		// LPC13xx definitions
#include "type.h"
#include "serial.h"


/*----------------------------------------------------------------------------
  Defines for ring buffers
 *---------------------------------------------------------------------------*/
#define SER_BUF_SIZE               (128)	// serial buffer in bytes (power 2)
#define SER_BUF_MASK               (SER_BUF_SIZE-1ul)	// buffer size mask

/* Buffer read / write macros */
#define SER_BUF_RESET(serBuf)      (serBuf.rdIdx = serBuf.wrIdx = 0)
#define SER_BUF_WR(serBuf, dataIn) (serBuf.data[SER_BUF_MASK & serBuf.wrIdx++] = (dataIn))
#define SER_BUF_RD(serBuf)         (serBuf.data[SER_BUF_MASK & serBuf.rdIdx++])
#define SER_BUF_EMPTY(serBuf)      (serBuf.rdIdx == serBuf.wrIdx)
#define SER_BUF_FULL(serBuf)       (serBuf.rdIdx == serBuf.wrIdx+1)
#define SER_BUF_COUNT(serBuf)      (SER_BUF_MASK & (serBuf.wrIdx - serBuf.rdIdx))

// buffer type
typedef struct __SER_BUF_T {
    unsigned char data[SER_BUF_SIZE];
    unsigned int wrIdx;
    unsigned int rdIdx;
} SER_BUF_T;

unsigned long ser_txRestart;	// NZ if TX restart is required
unsigned short ser_lineState;	// ((msr << 8) | (lsr))
SER_BUF_T ser_out;		// Serial data buffers
SER_BUF_T ser_in;

/*----------------------------------------------------------------------------
  open the serial port
 *---------------------------------------------------------------------------*/
void ser_OpenPort(void)
{

    NVIC_DisableIRQ(UART_IRQn);

    LPC_IOCON->PIO1_6 &= ~0x07;	/*  UART I/O config */
    LPC_IOCON->PIO1_6 |= 0x01;	/* UART RXD */
    LPC_IOCON->PIO1_7 &= ~0x07;
    LPC_IOCON->PIO1_7 |= 0x01;	/* UART TXD */
    /* Enable UART clock */
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 12);
    LPC_SYSCON->UARTCLKDIV = 0x1;	/* divided by 1 */
    return;
}

/*----------------------------------------------------------------------------
  close the serial port
 *---------------------------------------------------------------------------*/
void ser_ClosePort(void)
{
    LPC_IOCON->PIO1_6 &= ~0x07;	/*  UART I/O config */
    LPC_IOCON->PIO1_7 &= ~0x07;

    /* Disable the interrupt in the VIC and UART controllers */
    LPC_UART->IER = 0;
    NVIC_DisableIRQ(UART_IRQn);
    return;
}

/*----------------------------------------------------------------------------
  initialize the serial port
 *---------------------------------------------------------------------------*/
void ser_InitPort(unsigned long baudrate, unsigned int databits,
		  unsigned int parity, unsigned int stopbits)
{

    uint8_t lcr_p, lcr_s, lcr_d;
    uint32_t dll;
    uint32_t Fdiv;

    switch (databits) {
    case 5:			// 5 Data bits
	lcr_d = 0x00;
	break;
    case 6:			// 6 Data bits
	lcr_d = 0x01;
	break;
    case 7:			// 7 Data bits
	lcr_d = 0x02;
	break;
    case 8:			// 8 Data bits
    default:
	lcr_d = 0x03;
	break;
    }

    switch (stopbits) {
    case 1:			// 1,5 Stop bits
    case 2:			// 2   Stop bits
	lcr_s = 0x04;
	break;
    case 0:			// 1   Stop bit
    default:
	lcr_s = 0x00;
	break;
    }

    switch (parity) {
    case 1:			// Parity Odd
	lcr_p = 0x08;
	break;
    case 2:			// Parity Even
	lcr_p = 0x18;
	break;
    case 3:			// Parity Mark
	lcr_p = 0x28;
	break;
    case 4:			// Parity Space
	lcr_p = 0x38;
	break;
    case 0:			// Parity None
    default:
	lcr_p = 0x00;
	break;
    }

    SER_BUF_RESET(ser_out);	// reset out buffer
    SER_BUF_RESET(ser_in);	// reset in buffer

    /* Note that the pclk is 24,0 MHz.  (48.0 MHz / 2)         */
    /* 24 MHz PCLK generates also rates for 115200, 57600 baud */
    Fdiv = LPC_SYSCON->UARTCLKDIV;
    dll = (((SystemCoreClock / LPC_SYSCON->SYSAHBCLKDIV) / Fdiv) / 16) / baudrate;	/*baud rate */
    LPC_UART->FDR = 0;		// Fractional divider not used
    LPC_UART->LCR = 0x80 | lcr_d | lcr_p | lcr_s;	// Data bits, Parity,   Stop bit
    LPC_UART->DLL = dll;	// Baud Rate depending on PCLK
    LPC_UART->DLM = (dll >> 8);	// High divisor latch
    LPC_UART->LCR = 0x00 | lcr_d | lcr_p | lcr_s;	// DLAB = 0
    LPC_UART->IER = 0x03;	// Enable TX/RX interrupts

    LPC_UART->FCR = 0x07;	/* Enable and reset TX and RX FIFO. */
    ser_txRestart = 1;		// TX fifo is empty

    /* Enable the UART Interrupt */
    NVIC_EnableIRQ(UART_IRQn);
    return;
}

/*----------------------------------------------------------------------------
  read data from serial port
 *---------------------------------------------------------------------------*/
int ser_Read(char *buffer, const int *length)
{
    int bytesToRead, bytesRead;

    /* Read *length bytes, block if *bytes are not avaialable     */
    bytesToRead = *length;
    bytesToRead = (bytesToRead < (*length)) ? bytesToRead : (*length);
    bytesRead = bytesToRead;

    while (bytesToRead--) {
	while (SER_BUF_EMPTY(ser_in));	// Block until data is available if none
	*buffer++ = SER_BUF_RD(ser_in);
    }
    return (bytesRead);
}

/*----------------------------------------------------------------------------
  write data to the serial port
 *---------------------------------------------------------------------------*/
int ser_Write(const char *buffer, int *length)
{
    int bytesToWrite, bytesWritten;

    // Write *length bytes
    bytesToWrite = *length;
    bytesWritten = bytesToWrite;

    while (!SER_BUF_EMPTY(ser_out));	// Block until space is available if none
    while (bytesToWrite) {
	SER_BUF_WR(ser_out, *buffer++);	// Read Rx FIFO to buffer  
	bytesToWrite--;
    }

    if (ser_txRestart) {
	ser_txRestart = 0;
	LPC_UART->THR = SER_BUF_RD(ser_out);	// Write to the Tx Register
    }

    return (bytesWritten);
}

/*----------------------------------------------------------------------------
  check if character(s) are available at the serial interface
 *---------------------------------------------------------------------------*/
void ser_AvailChar(int *availChar)
{

    *availChar = SER_BUF_COUNT(ser_in);

}

/*----------------------------------------------------------------------------
  read the line state of the serial port
 *---------------------------------------------------------------------------*/
void ser_LineState(unsigned short *lineState)
{

    *lineState = ser_lineState;
    ser_lineState = 0;

}

/*----------------------------------------------------------------------------
  serial port 1 interrupt
 *---------------------------------------------------------------------------*/
void UART_IRQHandler(void)
{
    volatile unsigned long iir;

    iir = LPC_UART->IIR;

    if ((iir & 0x4) || (iir & 0xC)) {	// RDA or CTI pending
	while (LPC_UART->LSR & 0x01) {	// Rx FIFO is not empty
	    SER_BUF_WR(ser_in, LPC_UART->RBR);	// Read Rx FIFO to buffer  
	}
    }
    if ((iir & 0x2)) {		// TXMIS pending
	if (SER_BUF_COUNT(ser_out) != 0) {
	    LPC_UART->THR = SER_BUF_RD(ser_out);	// Write to the Tx FIFO
	    ser_txRestart = 0;
	} else {
	    ser_txRestart = 1;
	}
    }
    ser_lineState = LPC_UART->LSR & 0x1E;	// update linestate
    return;
}
