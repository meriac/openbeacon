#include <LPC13xx.h>
#include <config.h>
#include <gpio.h>
#include <uart.h>
#include <debug_printf.h>
#include "rfid.h"

extern volatile uint32_t UARTCount;
extern volatile uint8_t UARTBuffer[BUFSIZE];

int main(void)
{
    /* UART setup */
    UARTInit(115200);

    /* GPIO setup */
    GPIOInit();
    GPIOSetDir(LED_PORT, LED_BIT, 1);

    /* Init RFID */
    rfid_init();
}
