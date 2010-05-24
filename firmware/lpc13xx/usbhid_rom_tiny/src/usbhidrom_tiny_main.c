/*****************************************************************************
 *   usthidrom_tiny_main.c:  Tiny USBHID_ROM C file for
 *   NXP LPC13xx Family Microprocessors
 *
 *   Copyright(C) 2010, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2010.03.03  ver 1.00    Preliminary version, first Release
 *
*****************************************************************************/
#include "LPC13xx.h"		/* LPC13xx definitions */

#include "rom_drivers.h"
#include "config.h"

#define     EN_TIMER32_1    (1<<10)
#define     EN_IOCON        (1<<16)
#define     EN_USB_REG      (1<<14)

#define		SYSOSC_PD_BIT	5
#define		SYSOSC_PD_MASK	(1<<SYSOSC_PD_BIT)
#define		SYSPLL_PD_BIT	7
#define		SYSPLL_PD_MASK	(1<<SYSPLL_PD_BIT)
#define		USBPLL_PD_BIT	8
#define		USBPLL_PD_MASK	(1<<USBPLL_PD_BIT)
#define		USBPAD_PD_BIT	10
#define		USBPAD_PD_MASK	(1<<USBPAD_PD_BIT)

/*
 *  Get HID Input Report -> InReport
 */

void GetInReport(uint8_t src[], uint32_t length)
{
    (void)length;

    /* Check the input port bits and set the bits of
     * the "InReport" response for the PC. Only implements
     * one input bit.
     */
    src[0] = 0;
/*    if ((INPUT0_CPORT->DATA & (1 << INPUT0_BIT)) == INPUT_CLOSED)
	src[0] = 1;*/
}

/*
 *  Set HID Output Report <- OutReport
 */
void SetOutReport(uint8_t dst[], uint32_t length)
{
    (void)length;

    // Sets one I/O line based on bit 0 of the out report
    if (dst[0] & 1)
	OUTPUT0_CPORT->MASKED_ACCESS[1 << OUTPUT0_BIT] =
	    OUTPUT_ON << OUTPUT0_BIT;
    else
	OUTPUT0_CPORT->MASKED_ACCESS[1 << OUTPUT0_BIT] =
	    ~(OUTPUT_ON << OUTPUT0_BIT);
}

#define USB_HID 3
#define USB_MSC 8

const HID_DEVICE_INFO HidDevInfo = {
    USB_VENDOR_ID, USB_PROD_ID, USB_DEVICE,
    0,				// We are not providing a USB String Descriptor to save a little space
    1,				// In report count- 1 byte per report
    1,				// Out report count- 1 byte per report
    0x20,			// Sample interval- 32 mS
    GetInReport,		// Function pointer- generate data for PC
    SetOutReport		// Function pointer- process data from PC
};

const USB_DEV_INFO DeviceInfo = {
    USB_HID,
    (uint32_t) & HidDevInfo
};

#define rom ((const ROM **) 0x1fff1ff8)
const USBD *pUSBJumpTable;

__attribute__ ((section(".vectorcode")))
int main(void)
{
    // Enable Timer32_1 and IOCON (for USB ROM driver)
    LPC_SYSCON->SYSAHBCLKCTRL |= (EN_TIMER32_1 | EN_IOCON | EN_USB_REG);

    // Initialize ROM jump table pointer
    pUSBJumpTable = (*rom)->pUSBD;

    /* Use pll and pin init function in rom. This inits the main PLL
     * to 48 MHz and uses it to drive the USB module. */
    pUSBJumpTable->init_clk_pins();

    // Initialize output port pins
    OUTPUT0_CPORT->DIR |= 1 << OUTPUT0_BIT;

    pUSBJumpTable->init((USB_DEV_INFO *) & DeviceInfo);	/* USB Initialization */
    pUSBJumpTable->connect(TRUE);	/* USB Connect */

    while (1);
}

// USB Interrupt vector- calls into ROM
__attribute__ ((section(".vectorcode")))
void USB_IRQHandler(void)
{
    pUSBJumpTable->isr();
}
