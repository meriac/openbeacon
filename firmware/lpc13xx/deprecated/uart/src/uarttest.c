/*****************************************************************************
 *   uarttest.c:  UART test C file for NXP LPC13xx Family Microprocessors
 *
 *   Copyright(C) 2008, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2008.08.20  ver 1.00    Preliminary version, first Release
 *
******************************************************************************/

#include <LPC13xx.h>
#include <config.h>
#include <gpio.h>
#include <uart.h>
#include <debug_printf.h>

extern volatile uint32_t UARTCount;
extern volatile uint8_t UARTBuffer[BUFSIZE];

int main(void)
{
    volatile int i;
    /* Basic chip initialization is taken care of in SystemInit() called
     * from the startup code. SystemInit() and chip settings are defined
     * in the CMSIS system_<part family>.c file.
     */

    /* NVIC is installed inside UARTInit file. */
    UARTInit(115200);

    /* Initialize GPIO (sets up clock) */
    GPIOInit();

    /* Set LED port pin to output */
    GPIOSetDir(LED_PORT, LED_BIT, 1);

    while (1) {			/* Loop forever */
	if (UARTCount != 0) {
	    LPC_UART->IER = IER_THRE | IER_RLS;	/* Disable RBR */
	    UARTSend((uint8_t *) UARTBuffer, UARTCount);
	    UARTCount = 0;
	    LPC_UART->IER = IER_THRE | IER_RLS | IER_RBR;	/* Re-enable RBR */

	    debug_printf("Hello World!\n");

	    GPIOSetValue(LED_PORT, LED_BIT, LED_ON);
	    for(i=0;i<10000;i++);
	    GPIOSetValue(LED_PORT, LED_BIT, LED_OFF);
	}
    }
}
