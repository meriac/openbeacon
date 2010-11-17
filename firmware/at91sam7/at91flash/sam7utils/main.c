/*
 * main.c
 *
 * Copyright (C) 2005 Erik Gilling, all rights reserved
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 */

#include "config.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#include <readline/readline.h>
#include <readline/history.h>

#include <getopt.h>

#include "io.h"
#include "samba.h"
#include "cmd.h"

#define SAM7_TTY "/dev/at91_0"


static int sam7_parse_line( char *line, char *argv[] );
static int sam7_handle_line( char *line );
static void usage( void );

#define MAX_CMDS 32

int main( int argc, char *argv[] )
{
  char *cmdline;
  char *line = NULL;
  char *cmds[MAX_CMDS];
  int n_cmds=0;
  int c,i;


  while( 1 ) {
    int option_index;
    static struct option long_options[] = {
      { "line", 1, 0, 'l' },
      { "exec", 1, 0, 'e' },
      { "help", 0, 0, 'h' }
    };

    c = getopt_long( argc, argv, "l:e:h", long_options, &option_index );
    
    if( c == -1 ) {
      break;
    }

    switch( c ) {
    case 'l':
      line = optarg;
      break;
      
    case 'e':
      if( n_cmds >= MAX_CMDS ) {
	printf( "to many commands, increase MAX_CMDS\n" );
	return 1;
      }
      cmds[n_cmds] = optarg;
      n_cmds++;
      break;
      
    case 'h':
      usage();
      return 0;
      break;

    default:
      printf( "unknown option\n" );
      usage();
      return 1;
    }
  }


  if( io_init( line ) < 0 ) {
    return 1;
  }

  if( n_cmds > 0 ) {
    for( i=0 ; i<n_cmds ; i++ ) {
      sam7_handle_line( cmds[i] );
    }

  } else {
    while( (cmdline = readline( "sam7> " )) ) {
      add_history( cmdline );
      sam7_handle_line( cmdline );
    }
  }

  io_cleanup();

  return 0;
}

static void usage( void ) 
{
  printf( "usage: sam7 [options]\n" );
  printf( "  -e|--exec <cmd>   execute cmd instead of entering interactive mode.\n");
  printf( "  -l|--line <line>  specifies the line/tty which the boot agent resides.\n");
  printf( "                     only used for posix IO.\n" );
  printf( "  -h|--help         dislay this message and exit.\n");
}

static int sam7_parse_line( char *line, char *argv[] )
{
  char *p = line;
  int argc = 0;


  while( *p ) {
    // eat white space
    while( *p && isspace( *p ) ) { p++; }
    if( ! *p ) {
      break;
    }
    
    argv[argc++] = p;
    
    while( *p && !isspace( *p ) ) { p++; }
    if( *p ) {
      *p++ = '\0';
    }
  }
  
  return argc;
}

static int sam7_handle_line( char *line )
{
  int argc;
  char buff[256];
  char *argv[32];
  cmd_t *cmd;

  strncpy( buff, line, 255 );
  buff[255] = '\0';

  argc = sam7_parse_line( buff, argv );

  if( argc > 0 ) {
    if( (cmd = cmd_find( argv[0] )) != NULL ) {
      return cmd->func( argc, argv );
    }
    printf( "command %s not found\n", argv[0] );
    return -1;
  }
  
  return 0;
}

