/*----------------------------------------------------------------------------
 *      U S B  -  K e r n e l
 *----------------------------------------------------------------------------
 *      Name:    mscuser.c
 *      Purpose: Mass Storage Class Custom User Module
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
 *      Copyright (c) 2009 Keil - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/
#include "LPC13xx.h"
#include "type.h"
#include "usb.h"
#include "msccallback.h"
#include "diskimg.h"


const uint8_t InquiryStr[] = { 'K', 'e', 'i', 'l', ' ', ' ', ' ', ' ',
    'L', 'P', 'C', '1', '3', 'x', 'x', ' ',
    'D', 'i', 's', 'k', ' ', ' ', ' ', ' ',
    '1', '.', '0', ' ',
};

void MSC_MemoryRead(uint32_t offset, uint8_t dst[], uint32_t length)
{
    uint32_t n;

    for (n = 0; n < length; n++) {
	dst[n] = DiskImage[offset + n];
    }
}


/*
 *  MSC Memory Write Callback
 *   Called automatically on Memory Write Event
 *    Parameters:      None (global variables)
 *    Return Value:    None
 */

void MSC_MemoryWrite(uint32_t offset, uint8_t src[], uint32_t length)
{
    (void) offset;
    (void) src;
    (void) length;
}
