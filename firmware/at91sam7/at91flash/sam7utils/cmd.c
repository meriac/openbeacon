/*
 * cmd.c
 *
 * Copyright (C) 2005 Erik Gilling, all rights reserved.
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 */

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "samba.h"
#include "cmd.h"

#include "loader128_data.h"
#include "loader256_data.h"

#define MANUAL_FLASH

static int cmd_help( int argc, char *argv[] );
static int cmd_version( int argc, char *argv[] );
static int cmd_write_mem( int argc, char *argv[] );
static int cmd_read_mem( int argc, char *argv[] );
static int cmd_send_file( int argc, char *argv[] );
static int cmd_set_clock( int argc, char *argv[] );
static int cmd_locked_regions( int argc, char *argv[] );
static int cmd_unlock_regions( int argc, char *argv[] );
static int cmd_boot_from_flash( int argc, char *argv[] );
static int cmd_flash( int argc, char *argv[] );
static int cmd_read( int argc, char *argv[] );
static int cmd_manual_flash( int argc, char *argv[] );
static int cmd_manual_read( int argc, char *argv[] );

static cmd_t cmds[] = {
  {cmd_write_mem, "wb", 
   "wb <addr> <byte>",  "write <byte> to <addr>"},

  {cmd_read_mem,  "rb", 
   "rb <addr>",         "read byte from <addr>"},

  {cmd_write_mem, "ws", 
   "ws <addr> <short>", "write <short> to <addr>"},

  {cmd_read_mem,  "rs", 
   "rs <addr>",         "read short from <addr>"},

  {cmd_write_mem, "ww", 
   "ww <addr> <word>",  "write <word> to <addr>"},

  {cmd_read_mem,  "rw", 
   "rw <addr>",         "read word from <addr>"},

  {cmd_send_file, "sf",
   "sf <addr> <file> <offset> <len>", 
   "send len bytes of file starting at offset"},

  {cmd_set_clock, "set_clock",
   "set_clock",         "set clock as SAM-BA does"},
  {cmd_locked_regions, "locked_regions",
   "locked_regions",    "display locked regions"},
  {cmd_unlock_regions, "unlock_regions",
   "unlock_regions",    "unlock all lock regions"},
  {cmd_boot_from_flash, "boot_from_flash",
   "boot_from_flash",    "set the SAM7X to boot from flash"},
  {cmd_flash, "flash",
   "flash <file>", "upload <file> to flash using the loader"},   
  {cmd_read, "read",
   "read <file> <addr> <len>", "read <len> bytes at <addr> to <file> using R commands"},   
  {cmd_manual_flash, "manual_flash",
   "manual_flash <file>", "upload <file> to flash using ww commands"},   
  {cmd_manual_read, "manual_read",
   "manual_read <file> <addr> <len>", "write <len> bytes at <addr> to <file> using rw commands"},   
  {cmd_version, "version", 
   "version",           "get boot program version"},
  {cmd_help, "help", 
   "help",              "display help screen"}
};

cmd_t *cmd_find( char *name )
{
  int i;
  for( i=0 ; i<(sizeof(cmds)/
		sizeof(*cmds)); i++ ) {
    if( !strcmp( name, cmds[i].name ) ) {
      return &cmds[i];
    }
  }
  return NULL;
}

static int cmd_help( int argc, char *argv[] ) 
{
  int i;
  for( i=0 ; i<(sizeof(cmds)/
		sizeof(*cmds)); i++ ) {
    printf( "%-30s%s\n", cmds[i].invo, cmds[i].help );
  }

  return 0;
}

static int cmd_write_mem( int argc, char *argv[] )
{
  unsigned long int addr;
  unsigned long int val;
  char *endp;

  if( argc != 3 ) {
    return CMD_E_WRONG_NUM_ARGS;
  }
  
  addr = strtoul( argv[1], &endp, 0 );
  if( endp == argv[1] ||
      *endp != '\0' ) {
    return CMD_E_INVAL_ARG;
  }

  val = strtoul( argv[2], &endp, 0 );
  if( endp == argv[1] ||
      *endp != '\0' ) {
    return CMD_E_INVAL_ARG;
  }


  switch( argv[0][1] ) {
  case 'b':
    if( samba_write_byte( addr, (uint8_t) val ) < 0 ) {
      return CMD_E_IO_FAILURE;
    }
    break;
  case 's':
    if( samba_write_half_word( addr, (uint16_t) val ) < 0 ) {
      return CMD_E_IO_FAILURE;
    }
    break;
  case 'w':
    if( samba_write_word( addr, (uint32_t) val ) < 0 ) {
	return CMD_E_IO_FAILURE;
    }
    break;
  }

  return CMD_E_OK;
}

static int cmd_read_mem( int argc, char *argv[] )
{
  unsigned long int addr;
  char *endp;

  if( argc != 2 ) {
    return CMD_E_WRONG_NUM_ARGS;
  }
  
  addr = strtoul( argv[1], &endp, 0 );
  if( endp == argv[1] ||
      *endp != '\0' ) {
    return CMD_E_INVAL_ARG;
  }
  
  switch( argv[0][1] ) {
  case 'b':
    {
      uint8_t val;
      if( samba_read_byte( addr, &val ) < 0 ) {
	return CMD_E_IO_FAILURE;
      }
      printf( "%02X\n", val );
    }
    break;
  case 's':
    {
      uint16_t val;
      
      if( samba_read_half_word( addr, &val ) < 0 ) {
	return CMD_E_IO_FAILURE;
      }
      printf( "%04X\n", val );
    }
    break;
  case 'w':
    {
      uint32_t val;
      if( samba_read_word( addr, &val ) < 0 ) {
	return CMD_E_IO_FAILURE;
      }
      printf( "%08X\n", (unsigned int) val );
    }
    break;
  }
  
  return CMD_E_OK;
}

static int cmd_send_file( int argc, char *argv[] )
{
  unsigned long int offset;
  unsigned long int len;
  char *endp;
  uint8_t *data;
  int fd;

  if( argc != 5 ) {
    return CMD_E_WRONG_NUM_ARGS;
  }

  strtoul( argv[1], &endp, 0 );
  if( endp == argv[1] ||
      *endp != '\0' ) {
    return CMD_E_INVAL_ARG;
  }

  offset = strtoul( argv[3], &endp, 0 );
  if( endp == argv[1] ||
      *endp != '\0' ) {
    return CMD_E_INVAL_ARG;
  }

  len = strtoul( argv[4], &endp, 0 );
  if( endp == argv[1] ||
      *endp != '\0' ) {
    return CMD_E_INVAL_ARG;
  }

  if( (data = (uint8_t *) malloc( len )) == NULL ) {
    printf( "can't allocate %d bytes\n", (int)len );
    return CMD_E_NO_MEM;
  }

  if( (fd = open( argv[2], O_RDONLY )) < 0 ) {
    printf( "can't open file \"%s\": %s\n", argv[2],
	    strerror( errno ) );
    return CMD_E_BAD_FILE;
  }

  lseek( fd, offset, SEEK_SET );
  len = read( fd, data, len );

  if( samba_send_file( 0x202000, data, len ) < 0 ) {
    return CMD_E_IO_FAILURE;
  }

  return CMD_E_OK;
}

static int cmd_version( int argc, char *argv[] )
{
  char buff[256];

  if( samba_get_version( buff, 256 ) < 0 ) {
    return CMD_E_IO_FAILURE;
  }

  printf( "%s\n", buff );

  return CMD_E_OK;
}

static int cmd_set_clock( int argc, char *argv[] )
{
  uint32_t val;
  uint8_t val8;
  
  /* set clock to main clock / 2 */
  if( samba_write_word( 0xFFFFFC30, 0x5 ) < 0 ) {
    return -1;
  }
  
  do {
    if( samba_read_word( 0xFFFFFC68, &val ) < 0 ) {
      return -1;
    }
  } while( val != 0xD );

  /* set clock to pll clock / 2 */
  if( samba_write_word( 0xFFFFFC30, 0x7 ) < 0 ) {
    return -1;
  }

  do {
    if( samba_read_word( 0xFFFFFC68, &val ) < 0 ) {
      return -1;
    }
  } while( val != 0xD );


  /* read 0x200000 two times because SAM-BA does it */
  if( samba_read_byte( 0x00200000, &val8 ) < 0 ) {
    return -1;
  }
  if( val8 != 0x13 ) {
    printf( "warning: magic read 0x00200000 != 0x13\n" );
  }

  if( samba_read_byte( 0x00200000, &val8 ) < 0 ) {
    return -1;
  }
  if( val8 != 0x13 ) {
    printf( "warning: magic read 0x00200000 != 0x13\n" );
  }


  return 0;
}

static int cmd_locked_regions( int argc, char *argv[] )
{
  uint32_t val;
  int i;
  
  if( samba_read_word( 0xffffff68, &val ) < 0 ) {
    return -1;
  }
  

  printf( "Locked Regions:" );

  for( i=0 ; i<16 ; i++ ) {
    if( val & 0x00010000 ) {
      printf( " %i", i );
    }
    val >>= 1;
  }

  printf( "\n" );

  return 0;
}

static int cmd_unlock_regions( int argc, char *argv[] )
{
  uint32_t val;
  int i;
  
  if( samba_read_word( 0xffffff68, &val ) < 0 ) {
    return -1;
  }
  

  if( val & 0xffff0000 ) {
    /* set up MCLKS to some odd value... hey SAM-BA does it */
    if( samba_write_word( 0xffffff60, 0x00050100 ) < 0 ) {
      return -1;
    }
    
    for( i=0 ; i<samba_chip_info.lock_bits; i++ ) {
      if( val & 0x00010000 ) {
	int page = (samba_chip_info.nvpsiz / samba_chip_info.page_size /
		    samba_chip_info.lock_bits) * i;

	printf( "unlocking region %i:", i );
	
	
	if( samba_write_word( 0xffffff64, 
			      0x5a000004 | (page << 8) ) < 0 ) {
	  return -1;
	}

	printf( " done\n" );
      }
      val >>= 1;
    }


    /* set up MCLKS back to a sane value */
    if( samba_write_word( 0xffffff60, 0x00480100 ) < 0 ) {
      return -1;
    }
  }
  printf( "\n" );

  return 0;
}

/* contributed by Liam Staskawicz */
static int cmd_boot_from_flash( int argc, char *argv[] )
{
  /* 
   * word: 5A is key to send any message, 
   *       02 is GPNVM2 to boot from Flash, 
   *       0B is "set this bit"
   */
  uint32_t val;
  
  
  do {
    if( samba_read_word( 0xffffff68, &val ) < 0 ) {
      return -1;
    }
  } while( !val & 0x1 );

  if( samba_write_word( 0xFFFFFF64, 0x5A00020B ) < 0 ) {
    printf( "Couldn't flip the bit to boot from Flash.\n" );
    return -1;
  }
  return 0;
}

static int cmd_flash( int argc, char *argv[] )
{
  struct stat stbuf;
  size_t loader_len;
  size_t file_len;
  size_t i;
  uint8_t *buff;
  int file_fd;
  int read_len;
  int ps = samba_chip_info.page_size;
  uint8_t *loader_data;
  
  if( argc != 2 ) {
    return CMD_E_WRONG_NUM_ARGS;
  }

  if( ps == 128 ) {
    loader_data = loader128_data;
    loader_len = sizeof( loader128_data );
  } else if( ps == 256 ) {
    loader_data = loader256_data;
    loader_len = sizeof( loader256_data );
  } else {
    printf( "no loader for %d byte pages\n", ps );
    return -1;
  }

  if( stat( argv[1], &stbuf ) < 0 ) {
    printf( "%s not found\n", argv[1] );
    return -1;
  }

  file_len = stbuf.st_size;

  if( (buff = (uint8_t *) malloc( ps ) ) == NULL ) {
    printf( "can't alocate buffer of size 0x%x\n",  ps );
    goto error0;
  }

  if( samba_send_file( 0x00201600, loader_data, loader_len ) < 0 ) {
    printf( "could not upload samba.bin\n" );
    goto error1;
  }
  
  if( (file_fd = open( argv[1], O_RDONLY )) < 0 ) {
    printf( "can't open %s\n", argv[1] );
    return -1;
  }

  for( i=0 ; i<file_len ; i+=ps ) {
    /* set page # */
    if( samba_write_word( 0x00201400+ps, i/ps ) < 0 ) {
      printf( "could not write page %d address\n", (int) i/ps );
      goto error2;
    }
    
    read_len = (file_len-i < ps)?file_len-i:ps;
    /* XXX need to implement safe read */
    if( read( file_fd, buff, read_len ) < read_len ) {
      printf( "could not read 0x%x bytes from file\n", read_len );
      goto error2;
    }

    if( samba_send_file( 0x00201400, buff, ps ) < 0 ) {
      printf( "could not send page %d\n", (int) i/ps );
      goto error2;
    }

    if( samba_go( 0x00201600 ) < 0 ) {
      printf( "could not send go command for page %d\n", (int) i/ps );
      goto error2;
    }

  }
  
  free( buff );
  close( file_fd );

  return 0;

 error2:
  close( file_fd );

 error1:
  free( buff );

 error0:
  return -1;
}

static int cmd_manual_flash( int argc, char *argv[] )
{
  struct stat stbuf;
  size_t file_len;
  size_t i,j;
  char *buff;
  int file_fd;
  int read_len;
  uint32_t val;
  int ps = samba_chip_info.page_size;

  if( argc != 2 ) {
    return CMD_E_WRONG_NUM_ARGS;
  }

  if( stat( argv[1], &stbuf ) < 0 ) {
    printf( "%s not found\n", argv[1] );
    return -1;
  }
  file_len = stbuf.st_size;


  if( (buff = (char *) malloc( ps )) == NULL ) {
    printf( "can't alocate buffer of size 128\n" );
    return -1;
  }


  if( (file_fd = open( argv[1], O_RDONLY )) < 0 ) {
    printf( "can't open %s\n", argv[1] );
    goto error0;
  }

  for( i=0 ; i<file_len ; i+=ps ) {
    /* wait for flash to become ready */
    do {
      if( samba_read_word( 0xffffff68, &val ) < 0 ) {
	goto error1;
      }
    } while( !val & 0x1 );

    read_len = (file_len-i < ps)?file_len-i:ps;
    /* XXX need to implement safe read */
    if( read( file_fd, buff, read_len ) < read_len ) {
      printf( "could not read 0x%x bytes from file\n", read_len );
      goto error1;
    }

    for( j=0 ; j<ps ; j+=4 ) {
      if( samba_write_word( 0x00100000 + i + j, *((uint32_t*)(buff+j))) < 0 ) {
	printf( "could not write byte 0x%x\n", (unsigned int)( 0x00100000 + i + j) );
      }
    }

    /* send write command */
    if( samba_write_word( 0xFFFFFF64, 0x5A000001 | ((i/ps) << 8) ) < 0 ) {
      printf( "could not send write command for page %d\n", (int) i/ps );
      goto error1;
    }

  }
  
  free( buff );
  close( file_fd );

  return 0;

 error1:
  close( file_fd );

 error0:
  free( buff );
  
  return -1;
}

static int cmd_manual_read( int argc, char *argv[] )
{
  unsigned long int addr;
  unsigned long int len;
  int fd;
  int i;
  uint32_t val;
  char *endp;
  
  if( argc != 4 ) {
    printf( "wrong number of args\n" );
    return CMD_E_WRONG_NUM_ARGS;
  }
  
  addr = strtoul( argv[2], &endp, 0 );
  if( endp == argv[2] ||
      *endp != '\0' ) {
    printf("%s not a vaild number\n", argv[2]);
    return CMD_E_INVAL_ARG;
  }

  len = strtoul( argv[3], &endp, 0 );
  if( endp == argv[3] ||
      *endp != '\0' ) {
    printf("%s not a vaild number\n", argv[3]);
    return CMD_E_INVAL_ARG;
  }

  if( (fd = open( argv[1], O_RDWR | O_CREAT, 0666 )) < 0 ) {
    printf( "can't open file \"%s\": %s\n", argv[1],
	    strerror( errno ) );
    return CMD_E_BAD_FILE;
  }

  for( i=0; i<len ; i+=4 ) {
    if( samba_read_word( addr + i, &val ) < 0 ) {
      printf( "io error\n" );
      return CMD_E_IO_FAILURE; 
    }

    if( write( fd, &val, 4 ) < 4 ) {
      printf("write error\n" );
      return CMD_E_IO_FAILURE;
    }
  }
  
  close( fd );

  return CMD_E_OK;
}

static int cmd_read( int argc, char *argv[] )
{
  unsigned long int addr;
  unsigned long int len;
  int fd;
  int i;
  char *endp;
  int read_len;
  uint8_t buff[0x80];

  if( argc != 4 ) {
    printf( "wrong number of args\n");
    return CMD_E_WRONG_NUM_ARGS;
  }
  
  addr = strtoul( argv[2], &endp, 0 );
  if( endp == argv[2] ||
      *endp != '\0' ) {
    printf("%s not a vaild number\n", argv[2]);
    return CMD_E_INVAL_ARG;
  }

  len = strtoul( argv[3], &endp, 0 );
  if( endp == argv[3] ||
      *endp != '\0' ) {
    printf("%s not a vaild number\n", argv[3]);
    return CMD_E_INVAL_ARG;
  }

  if( (fd = open( argv[1], O_RDWR | O_CREAT, 0666 )) < 0 ) {
    printf( "can't open file \"%s\": %s\n", argv[1],
	    strerror( errno ) );
    return CMD_E_BAD_FILE;
  }

  for( i=0; i<len ; i+=0x80 ) {
    read_len = len-i < 0x80 ? len-i : 0x80;
    if( samba_recv_file( addr + i, buff, read_len ) < 0 ) {
      printf( "io error\n" );
      return CMD_E_IO_FAILURE; 
    }

    if( write( fd, buff, read_len ) < read_len ) {
      printf("write error\n" );
      return CMD_E_IO_FAILURE;
    }
  }
  
  close( fd );

  return CMD_E_OK;
}


