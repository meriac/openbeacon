/*----------------------------------------------------------------------------
 *      Name:    MEMORY.H
 *      Purpose: USB Memory Storage Demo Definitions
 *      Version: V1.10
 *----------------------------------------------------------------------------
 *      This file is part of the uVision/ARM development tools.
 *      This software may only be used under the terms of a valid, current,
 *      end user licence from KEIL for a compatible version of KEIL software
 *      development tools. Nothing else gives you the right to use it.
 *
 *      Copyright (c) 2005-2007 Keil Software.
 *---------------------------------------------------------------------------*/

/* MSC Disk Image Definitions */

/* Mass Storage Memory Layout */
#define MSC_BlockCount  13
#define MSC_BlockSize   512
#define MSC_MemorySize  (MSC_BlockCount*MSC_BlockSize)

extern const unsigned char DiskImage[MSC_MemorySize];	/* Disk Image */
