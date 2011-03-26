#ifndef __OPENBEACON_H__
#define __OPENBEACON_H__

#define PACKED __attribute__((packed))

#include <config.h>

#ifdef  __LPC13xx__
#include <LPC13xx.h>
#include <stdint.h>
#include <uart.h>
#ifndef ENALBLE_USB_FULLFEATURED
#include <usb.h>
#include <usbdesc.h>
#include <rom_drivers.h>
#endif/*ENALBLE_USB_FULLFEATURED*/
#include <gpio.h>
#ifdef  __LPC1343__
#define LPC_RAM_SIZE (8*1024)
#define LPC_FLASH_SIZE (32*1024)
#endif/*__LPC1343__*/
#ifdef  __LPC1342__
#define LPC_RAM_SIZE (4*1024)
#define LPC_FLASH_SIZE (16*1024)
#endif/*__LPC1342__*/
#else /*__LPC13xx__*/
#error Please specify architecture
#endif/*__LPC13xx__*/

#define DEVICEID_LPC1311 0x2C42502BUL
#define DEVICEID_LPC1313 0x2C40102BUL
#define DEVICEID_LPC1342 0x3D01402BUL
#define DEVICEID_LPC1343 0x3D00002BUL

#define TRUE 1
#define FALSE 0

#ifdef  ENABLE_FREERTOS
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#endif/*ENABLE_FREERTOS*/

#include <debug_printf.h>
#include <string.h>
#include <crc16.h>

static inline uint16_t htons(uint16_t x)
{
  __asm__ ("rev16 %0, %1" : "=r" (x) : "r" (x));
  return x;
}

static inline uint32_t htonl(uint32_t x)
{
  __asm__ ("rev %0, %1" : "=r" (x) : "r" (x));
  return x;
}

#define ntohl(l) htonl(l)
#define ntohs(s) htons(s)

#endif/*__OPENBEACON_H__*/
