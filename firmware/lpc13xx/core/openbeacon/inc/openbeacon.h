#ifndef __OPENBEACON_H__
#define __OPENBEACON_H__

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

#include <config.h>
#include <debug_printf.h>
#include <string.h>

#endif/*__OPENBEACON_H__*/