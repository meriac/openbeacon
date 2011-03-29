/*
 * samba.c
 *
 * Copyright (C) 2005,2006 Erik Gilling, all rights reserved
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 */

#include "config.h"

#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_ENDIAN_H
#include <endian.h>
#endif

#ifdef HAVE_LOCALENDIAN_H
#include <localendian.h>
#endif

#include "io.h"
#include "samba.h"


#ifndef __BYTE_ORDER
#  define __LITTLE_ENDIAN 4321
#  define __BIG_ENDIAN    1234
#  ifdef __LITTLE_ENDIAN__
#    define __BYTE_ORDER __LITTLE_ENDIAN
#  endif
#  ifdef __BIG_ENDIAN__
#    define __BYTE_ORDER __BIG_ENDIAN
#  endif
#endif

#if defined( __BYTE_ORDER )
#  if __BYTE_ORDER == __LITTLE_ENDIAN
#    define le2hl(x) (x)
#    define le2hs(x) (x)
#  endif

#  if __BYTE_ORDER == __BIG_ENDIAN

#    define le2hl(x)  \
  ((unsigned long int)((((unsigned long int)(x) & 0x000000ffU) << 24) | \
                       (((unsigned long int)(x) & 0x0000ff00U) <<  8) | \
                       (((unsigned long int)(x) & 0x00ff0000U) >>  8) | \
                       (((unsigned long int)(x) & 0xff000000U) >> 24)))

#    define le2hs(x) \
  ((unsigned short int)((((unsigned short int)(x) & 0x00ff) << 8) | \
                        (((unsigned short int)(x) & 0xff00) >> 8)))

#  endif
#endif

#define hl2le(x) le2hl(x)
#define hs2le(x) le2hs(x)

static const char *eprocs[] = {
  /* 000 */ "",
  /* 001 */ "ARM946E-S",
  /* 010 */ "ARM7TDMI",
  /* 011 */ "",
  /* 100 */ "ARM920T",
  /* 101 */ "ARM926EJ-S",
  /* 110 */ "",
  /* 111 */ ""
};


#define K 1024

static const int nvpsizs[] = {
  /* 0000 */ 0,
  /* 0001 */ 8 * K,
  /* 0010 */ 16 * K,
  /* 0011 */ 32 * K,
  /* 0100 */ -1,
  /* 0101 */ 64 * K,
  /* 0110 */ -1,
  /* 0111 */ 128 * K,
  /* 1000 */ -1,
  /* 1001 */ 256 * K,
  /* 1010 */ 512 * K,
  /* 1011 */ -1,
  /* 1100 */ 1024 * K,
  /* 1101 */ -1,
  /* 1110 */ 2048 * K,
  /* 1111 */ -1
};

static const int sramsizs[] = {
  /* 0000 */ -1,
  /* 0001 */ 1 * K,
  /* 0010 */ 2 * K,
  /* 0011 */ -1,
  /* 0100 */ 112 * K,
  /* 0101 */ 4 * K,
  /* 0110 */ 80 * K,
  /* 0111 */ 160 * K,
  /* 1000 */ 8 * K,
  /* 1001 */ 16 * K,
  /* 1010 */ 32 * K,
  /* 1011 */ 64 * K,
  /* 1100 */ 128 * K,
  /* 1101 */ 256 * K,
  /* 1110 */ 96 * K,
  /* 1111 */ 512 * K
};

static const struct { unsigned id; const char *name; } archs[] = {
  {AT91_ARCH_AT75Cxx,      "AT75Cxx"},
  {AT91_ARCH_AT91x40,      "AT91x40"},
  {AT91_ARCH_AT91x63,      "AT91x63"},
  {AT91_ARCH_AT91x55,      "AT91x55"},
  {AT91_ARCH_AT91x42,      "AT91x42"},
  {AT91_ARCH_AT91x92,      "AT91x92"},
  {AT91_ARCH_AT91x34,      "AT91x34"},
  {AT91_ARCH_AT91SAM7Axx,  "AT91SAM7Axx"},
  {AT91_ARCH_AT91SAM7Sxx,  "AT91SAM7Sxx"},
  {AT91_ARCH_AT91SAM7XC,   "AT91SAM7XC"},
  {AT91_ARCH_AT91SAM7SExx, "AT91SAM7SExx"},
  {AT91_ARCH_AT91SAM7Lxx,  "AT91SAM7Lxx"},
  {AT91_ARCH_AT91SAM7Xxx,  "AT91SAM7Xxx"},
  {AT91_ARCH_AT91SAM9xx,   "AT91SAM9xx"}
};

const char *at91_arch_str( int id ) {
  int i;
  for( i=0 ; i<(sizeof(archs)/sizeof(*archs)) ; i++ ) {
    if( archs[i].id == id ) {
      return archs[i].name;
    }
  }

  return "";
}

at91_chip_info_t samba_chip_info;

int samba_init( void ) 
{
  uint32_t chipid;
  uint16_t response;

  if( io_send_cmd( "N#", &response, sizeof( response ) ) < 0 ) {
    printf( "can't init boot program: %s\n", strerror( errno ) );
    return -1;
  }

  if( samba_read_word( 0xfffff240, &chipid ) < 0 ) {
    return -1;
  }

  samba_chip_info.version = AT91_CHIPID_VERSION( chipid );
  samba_chip_info.eproc = AT91_CHIPID_EPROC( chipid );
  samba_chip_info.nvpsiz = nvpsizs[AT91_CHIPID_NVPSIZ( chipid )];
  samba_chip_info.nvpsiz2 = nvpsizs[AT91_CHIPID_NVPSIZ2( chipid )];
  samba_chip_info.sramsiz = sramsizs[AT91_CHIPID_SRAMSIZ( chipid )];
  samba_chip_info.arch = AT91_CHIPID_ARCH( chipid );
  
  if( samba_chip_info.arch == AT91_ARCH_AT91SAM7Sxx ) {
    switch( samba_chip_info.nvpsiz) {
    case 32*K:
      samba_chip_info.page_size = 128;
      samba_chip_info.lock_bits = 8;
      break;

    case 64*K:
      samba_chip_info.page_size = 128;
      samba_chip_info.lock_bits = 16;
      break;

    case 128*K:
      samba_chip_info.page_size = 256;
      samba_chip_info.lock_bits = 8;
      break;

    case 256*K:
      samba_chip_info.page_size = 256;
      samba_chip_info.lock_bits = 16;
      break;

    default:
      printf( "unknown sam7s flash size %d\n", samba_chip_info.nvpsiz );
      return -1;
    }
  } else if( samba_chip_info.arch == AT91_ARCH_AT91SAM7SExx ) {

    switch( samba_chip_info.nvpsiz) {
    case 32*K:
      samba_chip_info.page_size = 128;
      samba_chip_info.lock_bits = 8;
      break;

    case 64*K:
      samba_chip_info.page_size = 128;
      samba_chip_info.lock_bits = 16;
      break;

    case 128*K:
      samba_chip_info.page_size = 256;
      samba_chip_info.lock_bits = 8;
      break;

    case 256*K:
      samba_chip_info.page_size = 256;
      samba_chip_info.lock_bits = 16;
      break;

    case 512*K:
      samba_chip_info.page_size = 256;
      samba_chip_info.lock_bits = 32;
      break;

    default:
      printf( "unknown sam7se flash size %d\n", samba_chip_info.nvpsiz );
      return -1;
  }
  } else if( (samba_chip_info.arch == AT91_ARCH_AT91SAM7Xxx) || (samba_chip_info.arch == AT91_ARCH_AT91SAM7XC) ) {

    switch( samba_chip_info.nvpsiz ) {
    case 128*K:
      samba_chip_info.page_size = 256;
      samba_chip_info.lock_bits = 8;
      break;

    case 256*K:
      samba_chip_info.page_size = 256;
      samba_chip_info.lock_bits = 16;
      break;

    default:
      printf( "unknown sam7x srflashsize %d\n", samba_chip_info.nvpsiz );
      return -1;
    }
  } else {
    printf( "Page size info of %s not known\n", 
	    at91_arch_str( samba_chip_info.arch ) );
    return -1;
  }

  printf("Chip Version: %d\n", samba_chip_info.version );
  printf("Embedded Processor: %s\n", eprocs[samba_chip_info.eproc] );
  printf("NVRAM Region 1 Size: %d K\n", samba_chip_info.nvpsiz / K );
  printf("NVRAM Region 2 Size: %d K\n", samba_chip_info.nvpsiz2 / K );
  printf("SRAM Size: %d K\n", samba_chip_info.sramsiz / K );
  printf("Series: %s\n", at91_arch_str( samba_chip_info.arch ) );
  printf("Page Size: %d bytes\n", samba_chip_info.page_size );
  printf("Lock Regions: %d\n", samba_chip_info.lock_bits );

  return 0;
}

int samba_write_byte( uint32_t addr, uint8_t value )
{
  char cmd[64];

  snprintf( cmd, 64, "O%08X,%02X#", (unsigned int) addr, 
	    (unsigned int) value );

  return io_send_cmd( cmd, NULL, 0 );
}

int samba_read_byte( uint32_t addr, uint8_t *value )
{
  char cmd[64];

  snprintf( cmd, 64, "o%08X,1#", (unsigned int) addr );
  
  return io_send_cmd( cmd, value, 1 );
}

int samba_write_half_word( uint32_t addr, uint16_t value )
{
  char cmd[64];

  snprintf( cmd, 64, "H%08X,%04X#", (unsigned int) addr, 
	    (unsigned int) value );


  return io_send_cmd( cmd, NULL, 0 );
}

int samba_read_half_word( uint32_t addr, uint16_t *value )
{
  char cmd[64];
  int err;

  snprintf( cmd, 64, "h%X,2#", (unsigned int) addr );
  
  err = io_send_cmd( cmd, value, 2 );
  *value = le2hs( *value );

  return err;

}

int samba_write_word( uint32_t addr, uint32_t value )
{
  char cmd[64];

  snprintf( cmd, 64, "W%08X,%08X#", (unsigned int) addr,
	    (unsigned int) value );

  return io_send_cmd( cmd, NULL, 0 );
}

int samba_read_word( uint32_t addr, uint32_t *value )
{
  char cmd[64];
  int err;

  snprintf( cmd, 64, "w%08X,4#", (unsigned int) addr );

  err = io_send_cmd( cmd, value, 4 );
  *value = le2hl( *value );

  return err; 
}


int samba_send_file( uint32_t addr, uint8_t *buff, int len )
{
  char cmd[64];
  int i=0;

  snprintf( cmd, 64, "S%X,%X#", (unsigned int) addr, (unsigned int) len );

  if( io_write( cmd, strlen( cmd ) ) < 0 ) {
    return -1;
  }

  usleep( 2000 );
 
  for( i=0 ; i<len ; i+=64 ) {
    if( io_write( buff+i, (len-i < 64)? len-i : 64 ) < 0 ){
      return -1;
    }
    usleep( 2000 );
  }
  
  return 0;
}

int samba_recv_file( uint32_t addr, uint8_t *buff, int len )
{
  char cmd[64];
  int i=0;

  snprintf( cmd, 64, "R%X,%X#", (unsigned int) addr, (unsigned int) len );

  if( io_write( cmd, strlen( cmd ) ) < 0 ) {
    return -1;
  }
  
  usleep( 2000 );

  for( i=0 ; i<len ; i+=64 ) {
    if( io_read( buff+i, (len-i < 64)? len-i : 64 ) < 0 ){
      return -1;
    }
  }
  
  return 0;

}

int samba_go( uint32_t addr )
{
  char cmd[64];
  snprintf( cmd, 64, "G%08X#", (unsigned int) addr );

  return io_send_cmd( cmd, NULL, 0 );
}

int samba_get_version( char *version, int len )
{
#if 0
  if( io_send_cmd( "V#", version, len ) < 0 ) {
    return -1;
  }

  return 0;
#else
  /* don't know the new form of V# */
  return -1;
#endif
}

