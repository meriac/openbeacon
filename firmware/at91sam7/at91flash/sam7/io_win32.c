/*
 * io_win32.c
 *
 * Copyright (C) 2005 Erik Gilling, all rights reserved
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 *
 * USB device enumeration modeled from http://www.delcom-eng.com/downloads/USBPRGMNL.pdf
 *
 */

#include "config.h"

#define WINVER 0x0500
#define INITGUID
#include <windows.h>
#include <setupapi.h>
#include <basetyps.h>

#include <stdio.h>

#include "io.h"
#include "samba.h"


// {E6EF7DCD-1795-4a08-9FBF-AA78423C26F0}
DEFINE_GUID(USBIODS_GUID, 
0xe6ef7dcd, 0x1795, 0x4a08, 0x9f, 0xbf, 0xaa, 0x78, 0x42, 0x3c, 0x26, 0xf0);

HANDLE io_pipe_in, io_pipe_out;

int io_init( char *dev )
{
  char *devname;
  
  //  char buff[16];

  HDEVINFO hInfo = SetupDiGetClassDevs(&USBIODS_GUID, NULL,
				       NULL, DIGCF_PRESENT | 
				       DIGCF_INTERFACEDEVICE);

  SP_INTERFACE_DEVICE_DATA Interface_Info;
  Interface_Info.cbSize = sizeof(Interface_Info);
  // Enumerate device
  if( !SetupDiEnumDeviceInterfaces(hInfo, NULL, (LPGUID)
				   &USBIODS_GUID,0, &Interface_Info ) ) {
    printf( "can't find boot agent\n");
    goto error0;
  }

  DWORD needed; // get the required lenght
  SetupDiGetInterfaceDeviceDetail(hInfo, &Interface_Info,
				  NULL, 0, &needed, NULL);
  PSP_INTERFACE_DEVICE_DETAIL_DATA detail =
    (PSP_INTERFACE_DEVICE_DETAIL_DATA) malloc(needed);
  if( !detail ) {
    printf( "can't find boot agent\n");
    goto error0;
    return(-1);
  }

  // fill the device details
  detail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

  if( !SetupDiGetInterfaceDeviceDetail( hInfo,
					&Interface_Info,detail, 
					needed,NULL, NULL )) {
    printf( "can't find boot agent\n");
    goto error1;
  }
  
  devname = (char *) malloc( strlen( detail->DevicePath ) + 7 );

  strcpy( devname, detail->DevicePath );
  strcat( devname, "\\PIPE00" );
  printf( "%s\n", devname );

  // open out

  io_pipe_out = CreateFile( devname, GENERIC_READ | GENERIC_WRITE,
			    FILE_SHARE_READ, NULL, OPEN_EXISTING,
			    0, NULL );

  if( io_pipe_out == INVALID_HANDLE_VALUE ) {
    printf( "can't open output pipe\n" );
	    goto error2;
  }

  strcpy( devname, detail->DevicePath );
  strcat( devname, "\\PIPE01" );
  
  // open in

  io_pipe_in = CreateFile( devname, GENERIC_READ | GENERIC_WRITE,
			    FILE_SHARE_READ, NULL, OPEN_EXISTING,
			    0, NULL );

  if( io_pipe_in == INVALID_HANDLE_VALUE ) {
    printf( "can't open input pipe\n" );
	    goto error3;
  }
  



  free((PVOID) detail);
  free( devname );

  
  
  return samba_init();

  return 0;
  //  return samba_init();

 error3:
  CloseHandle( io_pipe_out );

 error2:
  free( devname );
  
 error1:
  free((PVOID) detail);

 error0:
  SetupDiDestroyDeviceInfoList(hInfo);
  return -1;
}

int io_cleanup( void )
{
  CloseHandle( io_pipe_out );
  CloseHandle( io_pipe_in );

  return 0;
}
int io_write( void *buff, int len ) 
{
  int write_len = 0;
  DWORD bytes_written;
  
  while( write_len < len ) {
    
    if( !WriteFile( io_pipe_out, buff + write_len, 
		   len - write_len, &bytes_written, NULL ) ) {
      return -1;
    }
    write_len += bytes_written;
  }

  return write_len;
}

int io_read( void *buff, int len ) 
{
  int read_len = 0;
  DWORD bytes_read;
  
  while( read_len < len ) {
    
    if( !ReadFile( io_pipe_in, buff + read_len, 
		   len - read_len, &bytes_read, NULL ) ) {
      return -1;
    }
    read_len += bytes_read;
  }

  return read_len;
}

