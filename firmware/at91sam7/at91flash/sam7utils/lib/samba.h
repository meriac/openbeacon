/*
 * samba.h
 *
 * Copyright (C) 2005 Erik Gilling, all rights reserved
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 */

#ifndef __samba_h__
#define __samba_h__


#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#define AT91_CHIPID_VERSION( chipid ) (((chipid)>>0)&0x1f)
#define AT91_CHIPID_EPROC( chipid ) (((chipid)>>5)&0x7)
#define AT91_CHIPID_NVPSIZ( chipid ) (((chipid)>>8)&0xf)
#define AT91_CHIPID_NVPSIZ2( chipid ) (((chipid)>>12)&0xf)
#define AT91_CHIPID_SRAMSIZ( chipid ) (((chipid)>>16)&0xf)
#define AT91_CHIPID_ARCH( chipid ) (((chipid)>>20)&0xff)
#define AT91_CHIPID_NVPTYP( chipid ) (((chipid)>>28)&0x7)
#define AT91_CHIPID_EXT( chipid ) (((chipid)>>31)&0x1)

#define AT91_ARCH_AT75Cxx      0xF0
#define AT91_ARCH_AT91x40      0x40
#define AT91_ARCH_AT91x63      0x63
#define AT91_ARCH_AT91x55      0x55
#define AT91_ARCH_AT91x42      0x42
#define AT91_ARCH_AT91x92      0x92
#define AT91_ARCH_AT91x34      0x34
#define AT91_ARCH_AT91SAM7Axx  0x60
#define AT91_ARCH_AT91SAM7Sxx  0x70
#define AT91_ARCH_AT91SAM7XC   0x71
#define AT91_ARCH_AT91SAM7SExx 0x72
#define AT91_ARCH_AT91SAM7Lxx  0x73
#define AT91_ARCH_AT91SAM7Xxx  0x75
#define AT91_ARCH_AT91SAM9xx   0x19

typedef struct {
  int version;
  int eproc;
  int nvpsiz;
  int nvpsiz2;
  int sramsiz;
  int arch;
  int page_size;
  int lock_bits;
} at91_chip_info_t;

extern at91_chip_info_t samba_chip_info;

int samba_init( void );
int samba_write_byte( uint32_t addr, uint8_t value );
int samba_read_byte( uint32_t addr, uint8_t *value );
int samba_write_half_word( uint32_t addr, uint16_t value );
int samba_read_half_word( uint32_t addr, uint16_t *value );
int samba_write_word( uint32_t addr, uint32_t value );
int samba_read_word( uint32_t addr, uint32_t *value );
int samba_send_file( uint32_t addr, uint8_t *buff, int len );
int samba_recv_file( uint32_t addr, uint8_t *buff, int len );
int samba_go( uint32_t addr );
int samba_get_version( char *version, int len );

#endif /* __samba_h__ */
