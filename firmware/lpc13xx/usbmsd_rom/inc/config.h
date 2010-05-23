/*****************************************************************************
 *   config.h:  config file for usbmsd_rom example for NXP LPC13xx Family
 *   Microprocessors
 *
 *   Copyright(C) 2008, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2009.12.22  ver 1.00    Preliminary version, first Release
 *
******************************************************************************/

/*
Overview:
	This example shows how to use the on-chip USB driver in ROM to
	implement a simple MSC (Mass Storage Class) USB peripheral.

How to use:
   Click the debug toolbar button.
   Click the go button.
   Plug the LPCXpresso's target side into a PC using a USB cable retrofit
   or a 3rd party base board.

   * You should see a disk drive on the PC called "LPC134x USB" containing
	 a .txt file.
*/

 /* Note: Our HID Demonstration software only works with our VID */
#define NXP_VID		  0x1FC9
#define MY_VID	  	  0x????

#define USB_VENDOR_ID NXP_VID	// Vendor ID
#define USB_PROD_ID   0x0003	// Product ID
#define USB_DEVICE    0x0101	// Device ID

/*********************************************************************************
**                            End Of File
*********************************************************************************/
