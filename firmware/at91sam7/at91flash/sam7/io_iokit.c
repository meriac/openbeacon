/*
 * io.c
 *
 * Copyright (C) 2005 Erik Gilling, all rights reserved
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 */

#include "config.h"

#include <stdio.h>

#include <mach/mach.h>

#include <CoreFoundation/CFNumber.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>


#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "io.h"
#include "samba.h"


static mach_port_t masterPort = 0;
static IOUSBDeviceInterface **usbDev = NULL;
static IOUSBInterfaceInterface **intf = NULL;
static UInt8 inPipeRef = 0;
static UInt8 outPipeRef = 0;

#undef DEBUG_IO


int do_intf(io_service_t usbInterfaceRef)
{
    IOReturn err;
    IOCFPlugInInterface **iodev;
    SInt32 score;
    UInt8 numPipes;
    int i;
    UInt8 direction, number, transferType, interval;
    UInt16 maxPacketSize;
    
    err = IOCreatePlugInInterfaceForService(usbInterfaceRef, 
					    kIOUSBInterfaceUserClientTypeID, 
					    kIOCFPlugInInterfaceID, 
					    &iodev, &score);
    if( err || !iodev ) {
      printf("unable to create plugin. ret = %08x, iodev = %p\n", err, iodev);
      return -1;
    }
    
    err = (*iodev)->QueryInterface(iodev, 
				   CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID),
				   (LPVOID)&intf);
    IODestroyPlugInInterface(iodev);
	
    if (err || !intf) {
      printf("unable to create a device interface. ret = %08x, intf = %p\n", 
	     err, intf);
      return -1;
    }

    err = (*intf)->USBInterfaceOpen(intf);
    if (err) {
      printf("unable to open interface. ret = %08x\n", err);
      return -1;
    }
    
    err = (*intf)->GetNumEndpoints(intf, &numPipes);
    if (err) {
      printf("unable to get number of endpoints. ret = %08x\n", err);
      return -1;
    }

    if (numPipes) {
      for (i=1; i <= numPipes; i++) {
	
	err = (*intf)->GetPipeProperties(intf, i, &direction, 
					 &number, &transferType,
					 &maxPacketSize, &interval);
	if (err) {
	    printf("unable to get pipe properties for pipe %d, err = %08x\n",
		   i, err);
	    continue;
	}

	if (transferType != kUSBBulk) {
	    continue;
	}
	
	if ((direction == kUSBIn) && !inPipeRef) {
	    inPipeRef = i;
	}
	if ((direction == kUSBOut) && !outPipeRef) {
	    outPipeRef = i;
	}
      }
    }

    if( !inPipeRef || !outPipeRef ) {
      (*intf)->USBInterfaceClose(intf);
      (*intf)->Release(intf);
      intf = NULL;
      return -1;
    }

    return 0;
}


static int do_dev( io_service_t usbDeviceRef )
{   
  IOReturn err;
  IOCFPlugInInterface **iodev;		// requires <IOKit/IOCFPlugIn.h>
  SInt32 score;
  UInt8 numConf;
  IOUSBConfigurationDescriptorPtr confDesc;
  IOUSBFindInterfaceRequest interfaceRequest;
  io_iterator_t	iterator;
  io_service_t usbInterfaceRef;
  
  err = IOCreatePlugInInterfaceForService(usbDeviceRef, 
					  kIOUSBDeviceUserClientTypeID,
					  kIOCFPlugInInterfaceID, &iodev, &score);
  if (err || !iodev) {
    printf("unable to create plugin. ret = %08x, iodev = %p\n",
	   err, iodev);
		return -1;
    }
  
  err = (*iodev)->QueryInterface(iodev, 
				 CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID),
				 (LPVOID)&usbDev);
  IODestroyPlugInInterface(iodev);				// done with this
  
  if (err || !usbDev) {
    printf("unable to create a device interface. ret = %08x, dev = %p\n",
	   err, usbDev);
    return -1;
  }
  
  err = (*usbDev)->USBDeviceOpen(usbDev);
  if (err) {
    printf("unable to open device. ret = %08x\n", err);
    return -1;
  }
  err = (*usbDev)->GetNumberOfConfigurations(usbDev, &numConf);
  if (err || !numConf) {
    printf("unable to obtain the number of configurations. ret = %08x\n", err);
    return -1;
  }

  err = (*usbDev)->GetConfigurationDescriptorPtr(usbDev, 0, &confDesc);			// get the first config desc (index 0)
  if (err) {
      printf("unable to get config descriptor for index 0\n");
      return -1;
  }
  

  err = (*usbDev)->SetConfiguration(usbDev, confDesc->bConfigurationValue);
  if (err) {
    printf("unable to set the configuration\n");
    return -1;
  }

  // requested class
  interfaceRequest.bInterfaceClass = kIOUSBFindInterfaceDontCare;
  // requested subclass
  interfaceRequest.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;		
  // requested protocol
  interfaceRequest.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;		
  // requested alt setting
  interfaceRequest.bAlternateSetting = kIOUSBFindInterfaceDontCare;		
  
  err = (*usbDev)->CreateInterfaceIterator(usbDev, &interfaceRequest, &iterator);
  if (err) {
    printf("unable to create interface iterator\n");
    return -1;
  }
    
  while( (usbInterfaceRef = IOIteratorNext(iterator)) ) {
    if( do_intf( usbInterfaceRef ) == 0 ) {
      IOObjectRelease(iterator);
      iterator = 0;
      return 0;
    }
  } 

    
  IOObjectRelease(iterator);
  iterator = 0;
  return -1;


}

int io_init( char *dev __attribute__((unused)) )
{

  kern_return_t err;
  CFMutableDictionaryRef matchingDictionary = 0;
  CFNumberRef numberRef;
  SInt32 idVendor = USB_VID_ATMEL;
  SInt32 idProduct = USB_PID_SAMBA;
  io_iterator_t iterator = 0;
  io_service_t usbDeviceRef;

  
  if( (err = IOMasterPort( MACH_PORT_NULL, &masterPort )) ) {
    printf( "could not create master port, err = %08x\n", err );
    return -1;
  }

  if( !(matchingDictionary = IOServiceMatching(kIOUSBDeviceClassName)) ) {
    printf( "could not create matching dictionary\n" );
    return -1;
  }
  
  if( !(numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type,
				   &idVendor)) ) {
    printf( "could not create CFNumberRef for vendor\n" );
    return -1;
  }
  CFDictionaryAddValue( matchingDictionary, CFSTR(kUSBVendorID), numberRef);
  CFRelease( numberRef );
  numberRef = 0;

  if( !(numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type,
				   &idProduct)) ) {
    printf( "could not create CFNumberRef for product\n" );
    return -1;
  }
  CFDictionaryAddValue( matchingDictionary, CFSTR(kUSBProductID), numberRef);
  CFRelease( numberRef );
  numberRef = 0;
  
  err = IOServiceGetMatchingServices( masterPort, matchingDictionary, &iterator );
  matchingDictionary = 0;  // consumed by the above call


  
  if( (usbDeviceRef = IOIteratorNext( iterator )) ) {
    printf( "found boot agent\n" );

    do_dev( usbDeviceRef );
  } else {
    printf( "can not find boot agent\n" );
    return -1;
  }



  IOObjectRelease(usbDeviceRef);

  IOObjectRelease(iterator);

  
  return samba_init();


  IOObjectRelease(usbDeviceRef);

  IOObjectRelease(iterator);
  return -1;
}


int io_cleanup( void ) 
{
  if( intf ) {
    (*intf)->USBInterfaceClose(intf);
    (*intf)->Release(intf);
    intf = NULL;
  }

  if( usbDev ) {
    (*usbDev)->USBDeviceClose(usbDev);
    (*usbDev)->Release(usbDev);
    usbDev = NULL;
  }
  
  return 0;
}

int io_write( void *buff, int len ) 
{
  if( (*intf)->WritePipe( intf, outPipeRef, buff, (UInt32) len ) !=
      kIOReturnSuccess ) {
    printf( "write error\n");
  }

  return len;
}

int io_read( void *buff, int len ) 
{

   UInt32 size = len;

  if( (*intf)->ReadPipe( intf, inPipeRef, buff, &size ) !=
      kIOReturnSuccess ) {
    printf( "read error\n");
  }

  
  return (int) size;
}

