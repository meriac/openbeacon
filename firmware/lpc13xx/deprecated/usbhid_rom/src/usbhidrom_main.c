/*----------------------------------------------------------------------------
 *      Name:    DEMO.C
 *      Purpose: USB HID Demo
 *      Version: V1.20
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

#include "LPC13xx.h"		/* LPC13xx definitions */

#include "usb.h"
#include "usbdesc.h"
#include "gpio.h"
#include "rom_drivers.h"
#include "config.h"

#define     EN_TIMER32_1    (1<<10)
#define     EN_IOCON        (1<<16)
#define     EN_USBREG       (1<<14)

USB_DEV_INFO DeviceInfo;
HID_DEVICE_INFO HidDevInfo;
ROM **rom = (ROM **) 0x1fff1ff8;

/*
 *  Get HID Input Report -> InReport
 */

void GetInReport(uint8_t src[], uint32_t length)
{
    (void) length;

    uint8_t PCInReportData = 0;

    /* Check the input port bits and set the bits of
     * the "InReport" response for the PC.
     */

#ifdef INPUT0_BIT
    if ((INPUT0_CPORT->DATA & (1 << INPUT0_BIT)) == INPUT_CLOSED)
	PCInReportData |= 1 << 0;
#endif

#ifdef INPUT1_BIT
    if ((INPUT1_CPORT->DATA & (1 << INPUT1_BIT)) == INPUT_CLOSED)
	PCInReportData |= 1 << 1;
#endif

#ifdef INPUT2_BIT
    if ((INPUT2_CPORT->DATA & (1 << INPUT2_BIT)) == INPUT_CLOSED)
	PCInReportData |= 1 << 2;
#endif

#ifdef INPUT3_BIT
    if ((INPUT3_CPORT->DATA & (1 << INPUT3_BIT)) == INPUT_CLOSED)
	PCInReportData |= 1 << 3;
#endif

#ifdef INPUT4_BIT
    if ((INPUT4_CPORT->DATA & (1 << INPUT4_BIT)) == INPUT_CLOSED)
	PCInReportData |= 1 << 4;
#endif

#ifdef INPUT5_BIT
    if ((INPUT5_CPORT->DATA & (1 << INPUT5_BIT)) == INPUT_CLOSED)
	PCInReportData |= 1 << 5;
#endif

#ifdef INPUT6_BIT
    if ((INPUT6_CPORT->DATA & (1 << INPUT6_BIT)) == INPUT_CLOSED)
	PCInReportData |= 1 << 6;
#endif

#ifdef INPUT7_BIT
    if ((INPUT7_CPORT->DATA & (1 << INPUT7_BIT)) == INPUT_CLOSED)
	PCInReportData |= 1 << 7;
#endif

    src[0] = PCInReportData;
}

/*
 *  Set HID Output Report <- OutReport
 */
void SetOutReport(uint8_t dst[], uint32_t length)
{
    (void) length;

    uint8_t PCOutReportData = dst[0];

    /* Check the bits of the "OutReport" data from the PC
     * and set the output port status.
     */

#ifdef OUTPUT0_BIT
    if (PCOutReportData & (1 << 0))
	GPIOSetValue(OUTPUT0_PORT, OUTPUT0_BIT, OUTPUT_ON);
    else
	GPIOSetValue(OUTPUT0_PORT, OUTPUT0_BIT, OUTPUT_OFF);
#endif

#ifdef OUTPUT1_BIT
    if (PCOutReportData & (1 << 1))
	GPIOSetValue(OUTPUT1_PORT, OUTPUT1_BIT, OUTPUT_ON);
    else
	GPIOSetValue(OUTPUT1_PORT, OUTPUT1_BIT, OUTPUT_OFF);
#endif

#ifdef OUTPUT2_BIT
    if (PCOutReportData & (1 << 2))
	GPIOSetValue(OUTPUT2_PORT, OUTPUT2_BIT, OUTPUT_ON);
    else
	GPIOSetValue(OUTPUT2_PORT, OUTPUT2_BIT, OUTPUT_OFF);
#endif

#ifdef OUTPUT3_BIT
    if (PCOutReportData & (1 << 3))
	GPIOSetValue(OUTPUT3_PORT, OUTPUT3_BIT, OUTPUT_ON);
    else
	GPIOSetValue(OUTPUT3_PORT, OUTPUT3_BIT, OUTPUT_OFF);
#endif

#ifdef OUTPUT4_BIT
    if (PCOutReportData & (1 << 4))
	GPIOSetValue(OUTPUT4_PORT, OUTPUT4_BIT, OUTPUT_ON);
    else
	GPIOSetValue(OUTPUT4_PORT, OUTPUT4_BIT, OUTPUT_OFF);
#endif

#ifdef OUTPUT5_BIT
    if (PCOutReportData & (1 << 5))
	GPIOSetValue(OUTPUT5_PORT, OUTPUT5_BIT, OUTPUT_ON);
    else
	GPIOSetValue(OUTPUT5_PORT, OUTPUT5_BIT, OUTPUT_OFF);
#endif

#ifdef OUTPUT6_BIT
    if (PCOutReportData & (1 << 6))
	GPIOSetValue(OUTPUT6_PORT, OUTPUT6_BIT, OUTPUT_ON);
    else
	GPIOSetValue(OUTPUT6_PORT, OUTPUT6_BIT, OUTPUT_OFF);
#endif

#ifdef OUTPUT7_BIT
    if (PCOutReportData & (1 << 7))
	GPIOSetValue(OUTPUT7_PORT, OUTPUT7_BIT, OUTPUT_ON);
    else
	GPIOSetValue(OUTPUT7_PORT, OUTPUT7_BIT, OUTPUT_OFF);
#endif
}

int main(void)
{
    /* for delay loop */
    volatile int n;
// Code Red Red Suite and LPCXpresso by Code Red both call SystemInit() in
// the C startup code
#ifndef __CODERED__
    SystemInit();
#endif


    HidDevInfo.idVendor = USB_VENDOR_ID;
    HidDevInfo.idProduct = USB_PROD_ID;
    HidDevInfo.bcdDevice = USB_DEVICE;
    HidDevInfo.StrDescPtr = (uint32_t) & USB_StringDescriptor[0];
    HidDevInfo.InReportCount = 1;
    HidDevInfo.OutReportCount = 1;
    HidDevInfo.SampleInterval = 0x20;
    HidDevInfo.InReport = GetInReport;
    HidDevInfo.OutReport = SetOutReport;

    DeviceInfo.DevType = USB_DEVICE_CLASS_HUMAN_INTERFACE;
    DeviceInfo.DevDetailPtr = (uint32_t) & HidDevInfo;

    /* Initialize input port pins */
    // This is not required in this specific app since GPIOnDIR = 0 on
    // reset, setting all I/O pins as input
#ifdef INPUT0_BIT
    GPIOSetDir(INPUT0_PORT, INPUT0_BIT, 0);
#endif

#ifdef INPUT1_BIT
    GPIOSetDir(INPUT1_PORT, INPUT1_BIT, 0);
#endif

#ifdef INPUT2_BIT
    GPIOSetDir(INPUT2_PORT, INPUT2_BIT, 0);
#endif

#ifdef INPUT3_BIT
    GPIOSetDir(INPUT3_PORT, INPUT3_BIT, 0);
#endif

#ifdef INPUT4_BIT
    GPIOSetDir(INPUT4_PORT, INPUT4_BIT, 0);
#endif

#ifdef INPUT5_BIT
    GPIOSetDir(INPUT5_PORT, INPUT5_BIT, 0);
#endif

#ifdef INPUT6_BIT
    GPIOSetDir(INPUT6_PORT, INPUT6_BIT, 0);
#endif

#ifdef INPUT7_BIT
    GPIOSetDir(INPUT7_PORT, INPUT7_BIT, 0);
#endif

    /* Initialize output port pins */
#ifdef OUTPUT0_BIT
    GPIOSetDir(OUTPUT0_PORT, OUTPUT0_BIT, 1);
#endif

#ifdef OUTPUT1_BIT
    GPIOSetDir(OUTPUT1_PORT, OUTPUT1_BIT, 1);
#endif

#ifdef OUTPUT2_BIT
    GPIOSetDir(OUTPUT2_PORT, OUTPUT2_BIT, 1);
#endif

#ifdef OUTPUT3_BIT
    GPIOSetDir(OUTPUT3_PORT, OUTPUT3_BIT, 1);
#endif

#ifdef OUTPUT4_BIT
    GPIOSetDir(OUTPUT4_PORT, OUTPUT4_BIT, 1);
#endif

#ifdef OUTPUT5_BIT
    GPIOSetDir(OUTPUT5_PORT, OUTPUT5_BIT, 1);
#endif

#ifdef OUTPUT6_BIT
    GPIOSetDir(OUTPUT6_PORT, OUTPUT6_BIT, 1);
#endif

#ifdef OUTPUT7_BIT
    GPIOSetDir(OUTPUT7_PORT, OUTPUT7_BIT, 1);
#endif


    /* Enable Timer32_1, IOCON, and USB blocks (for USB ROM driver) */
    LPC_SYSCON->SYSAHBCLKCTRL |= (EN_TIMER32_1 | EN_IOCON | EN_USBREG);

    /* Use pll and pin init function in rom */
    (*rom)->pUSBD->init_clk_pins();

    /* insert a delay between clk init and usb init */
    for (n = 0; n < 75; n++) {
    }

    (*rom)->pUSBD->init(&DeviceInfo);	/* USB Initialization */
    (*rom)->pUSBD->connect(TRUE);	/* USB Connect */

    while (1)
	__WFI();
}

#if defined(__IAR__)
void USBIRQ_IRQHandler(void)
#else
void USB_IRQHandler(void)
#endif
{
    (*rom)->pUSBD->isr();
}
