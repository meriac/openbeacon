/*****************************************************************************
 *   config.h:  config file for usbhid_rom example for NXP LPC13xx Family
 *   Microprocessors
 *
 *   Copyright(C) 2008, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2008.07.19  ver 1.00    Preliminary version, first Release
 *
******************************************************************************/

/*
Overview:
	This example shows how to use the on-chip USB driver in ROM to
	implement a simple HID (Human Interface Device) class USB peripheral.
	To run this example, you must attach a USB cable to the board. See
	the "Getting Started Guide" appendix for details.

User #defines:
   LED_PORT	I/O port driving an LED
   LED_BIT  I/O port bit driving an LED
   LED_ON   Value to set the bit to turn on the LED
   LED_OFF  Value to set the bit to turn off the LED
   LED_TOGGLE_TICKS
		    Number of timer ticks per flash. The timer is configured to generate
		    an interrupt 100 times a second or every 10mS.

How to use:
   Click the debug toolbar button.
   Click the go button.
   Plug the LPCXpresso's target side into a PC using a USB cable retrofit
   or a 3rd party base board.

   * You should be able to control the LED with LPC1343 HID Demonstration.exe
*/

 /* Note: Our HID Demonstration software only works with our VID */
#define NXP_VID		  0x1FC9
#define MY_VID	  	  0x????

#define USB_VENDOR_ID NXP_VID	// Vendor ID
#define USB_PROD_ID   0x0003	// Product ID
#define USB_DEVICE    0x0100	// Device ID

/* Define the output pins controlled by the USB HID client */
#if defined(__IAR__)
/* LED outputs for the LPC1343-SK board from IAR */
#define OUTPUT_ON               0
#define OUTPUT_OFF              (1-OUTPUT_ON)

#define OUTPUT0_PORT	PORT3
#define OUTPUT0_BIT		0

#define OUTPUT1_PORT	PORT1
#define OUTPUT1_BIT		9

#elif defined(__KEIL__)

/* LED outputs for the MCB1000 board from Keil */
#define OUTPUT_ON               0
#define OUTPUT_OFF              (1-OUTPUT_ON)

#define OUTPUT0_PORT	PORT2
#define OUTPUT0_BIT		0

#define OUTPUT1_PORT	PORT2
#define OUTPUT1_BIT		1

#define OUTPUT2_PORT	PORT2
#define OUTPUT2_BIT		2

#define OUTPUT3_PORT	PORT2
#define OUTPUT3_BIT		3

#define OUTPUT4_PORT	PORT2
#define OUTPUT4_BIT		4

#define OUTPUT5_PORT	PORT2
#define OUTPUT5_BIT		5

#define OUTPUT6_PORT	PORT2
#define OUTPUT6_BIT		6

#define OUTPUT7_PORT	PORT2
#define OUTPUT7_BIT		7

#else
#define OUTPUT_ON               1
#define OUTPUT_OFF              (1-OUTPUT_ON)

#define OUTPUT0_PORT	PORT0
#define OUTPUT0_BIT		7

#define OUTPUT1_PORT	PORT1
#define OUTPUT1_BIT		1

#endif



/* Define the input pins controlled by the USB HID client */

#if defined(__IAR__)
/* Pushbuttons for the LPC1343-SK board from IAR */
#define INPUT_CLOSED            0
#define INPUT_OPEN              (1-INPUT_CLOSED)

#define INPUT0_PORT		PORT0
#define INPUT0_CPORT	LPC_GPIO0
#define INPUT0_BIT		7

#define INPUT1_PORT		PORT1
#define INPUT1_CPORT	LPC_GPIO1
#define INPUT1_BIT		4

#elif defined(__KEIL__)
/* Pushbutton for the MCB1000 board from Keil */
#define INPUT_CLOSED            0
#define INPUT_OPEN              (1-INPUT_CLOSED)

#define INPUT0_PORT		PORT0
#define INPUT0_CPORT	LPC_GPIO0
#define INPUT0_BIT		1

#else
#define INPUT_CLOSED            0
#define INPUT_OPEN              (1-INPUT_CLOSED)

#define INPUT0_PORT		PORT0
#define INPUT0_CPORT	LPC_GPIO0
#define INPUT0_BIT		2

#define INPUT1_PORT		PORT1
#define INPUT1_CPORT	LPC_GPIO1
#define INPUT1_BIT		2

#define INPUT2_PORT		PORT2
#define INPUT2_CPORT	LPC_GPIO2
#define INPUT2_BIT		2

#define INPUT3_PORT		PORT3
#define INPUT3_CPORT	LPC_GPIO3
#define INPUT3_BIT		2

#define INPUT4_PORT		PORT0
#define INPUT4_CPORT	LPC_GPIO0
#define INPUT4_BIT		3

#define INPUT5_PORT		PORT1
#define INPUT5_CPORT	LPC_GPIO1
#define INPUT5_BIT		3

#define INPUT6_PORT		PORT2
#define INPUT6_CPORT	LPC_GPIO2
#define INPUT6_BIT		3

#define INPUT7_PORT		PORT3
#define INPUT7_CPORT	LPC_GPIO3
#define INPUT7_BIT		3

#endif

/*********************************************************************************
**                            End Of File
*********************************************************************************/
