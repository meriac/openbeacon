/*****************************************************************************
 *   compiler.h:  Compiler-specific header file for NXP Family 
 *   Microprocessors
 *
 *   Copyright(C) 2009, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2009.12.23  ver 1.00    Preliminary version, first Release
 *
******************************************************************************/
#ifndef __COMPILER_H__
#define __COMPILER_H__

#include "lpc13xx.h"		// Defines __ASM and __INLINE

#if !defined(__IAR__) && !defined(__KEIL__) && !defined(__CODERED__)

#if defined(__IAR_SYSTEMS_ICC__)
#define __IAR__
#endif

#if defined(__GNUC__)
#define __CODERED__
#endif

#if defined(__CC_ARM)
#define __KEIL__
#endif

#endif
/*
Various compiler declaration methods for packed structures:
GNU/IAR: typedef struct { blah blah blah } __attribute__((packed)) structName;
Keil: typedef __packed struct { blah blah blah } structName;
*/
#if defined(__GNUC__)
#define PACKED_PRE /**/
#define PACKED_POST __attribute__((packed))
#elif defined(__IAR_SYSTEMS_ICC__)
#define PACKED_PRE /**/
#define PACKED_POST __packed
#else				/* Keil? */
#define PACKED_PRE __packed
#define PACKED_POST /**/
#endif
#endif				/* __COMPILER_H__ */
