#ifndef __DEBUG_PRINTF_H__
#define __DEBUG_PRINTF_H__

#if defined UART_DISABLE && !defined ENABLE_USB_FULLFEATURED
#define debug_printf(...)
#else /*UART_DISABLE*/
extern void debug_printf (const char *fmt, ...);
extern char hex_char (unsigned char hex);
extern void hex_dump (const unsigned char *buf, unsigned int addr, unsigned int len);
#endif/*UART_DISABLE*/

#endif/*__DEBUG_PRINTF_H__*/
