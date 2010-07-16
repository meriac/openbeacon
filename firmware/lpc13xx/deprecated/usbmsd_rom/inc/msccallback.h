/*----------------------------------------------------------------------------
 *      U S B  -  K e r n e l
 *----------------------------------------------------------------------------
 *      Name:    MSCUSER.H
 *      Purpose: Mass Storage Class Custom User Definitions
 *      Version: V1.10
 *----------------------------------------------------------------------------
 *      This file is part of the uVision/ARM development tools.
 *      This software may only be used under the terms of a valid, current,
 *      end user licence from KEIL for a compatible version of KEIL software
 *      development tools. Nothing else gives you the right to use it.
 *
 *      Copyright (c) 2005-2007 Keil Software.
 *---------------------------------------------------------------------------*/

#ifndef __MSCUSER_H__
#define __MSCUSER_H__

extern const uint8_t InquiryStr[];

extern void MSC_MemoryRead(uint32_t offset, uint8_t dst[],
			   uint32_t length);
extern void MSC_MemoryWrite(uint32_t offset, uint8_t src[],
			    uint32_t length);


#endif				/* __MSCUSER_H__ */
