/*
 * io.h
 *
 * Copyright (C) 2005 Erik Gilling, all rights reserved
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 */

#ifndef __io_h__
#define __io_h__


#include <string.h>  // for strlen
#include <unistd.h>  // for usleep

#define USB_VID_ATMEL    0x03eb
#define USB_PID_SAMBA    0x6124


int io_init( char *dev );
int io_cleanup( void );
int io_write( void *buff, int len );
int io_read( void *buff, int len );
int io_read_response( char *buff, int len );


static inline int io_send_cmd( char *cmd, void *response, int response_len )
{
  
  if( io_write( cmd, strlen( cmd ) ) < 0 ) {
    return -1;
  }
  usleep( 2000 );

  if( response_len == 0 ) {
    return 0;
  }

  if( io_read( response, response_len ) < 0 ) {
    return -1;
  }

  usleep( 2000 );
  return 0;
}

#endif /* __io_h__ */
