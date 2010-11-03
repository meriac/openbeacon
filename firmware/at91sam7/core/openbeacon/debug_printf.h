/*
 *  linux/lib/vsprintf.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/* vsprintf.c -- Lars Wirzenius & Linus Torvalds. */
/*
 * Wirzenius wrote this portably, Torvalds fucked it up :-)
 */

#ifndef __DEBUG_PRINTF_H__
#define __DEBUG_PRINTF_H__

extern void debug_printf (const char *fmt, ...);
extern void hex_dump (const unsigned char *buf, unsigned int addr, unsigned int len);
extern void vDebugSendHook (char data);

#endif/*__DEBUG_PRINTF_H__*/
