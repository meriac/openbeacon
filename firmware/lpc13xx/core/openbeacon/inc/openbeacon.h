#ifndef __OPENBEACON_H__
#define __OPENBEACON_H__

#define PACKED __attribute__((packed))

#ifdef  __LPC13xx__
#include <LPC13xx.h>
#include <uart.h>
#include <usb.h>
#include <usbdesc.h>
#include <gpio.h>
#include <rom_drivers.h>
#else /*__LPC13xx__*/
#error Please specify architecture
#endif/*__LPC13xx__*/

#define DEVICEID_LPC1311 0x2C42502BUL
#define DEVICEID_LPC1313 0x2C40102BUL
#define DEVICEID_LPC1342 0x3D01402BUL
#define DEVICEID_LPC1343 0x3D00002BUL

#include <config.h>
#include <debug_printf.h>
#include <string.h>

#endif/*__OPENBEACON_H__*/