/*
 * cmd.h
 *
 * Copyright (C) 2005 Erik Gilling, all rights reserved
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 */

#ifndef __cmd_h__
#define __cmd_h__

typedef struct {
  int (* func)( int argc, char *argv[] );
  char *name;
  char *invo;
  char *help;
} cmd_t;


#define CMD_E_OK               0
#define CMD_E_WRONG_NUM_ARGS  -1
#define CMD_E_INVAL_ARG       -2
#define CMD_E_IO_FAILURE      -3
#define CMD_E_UNIMP           -4
#define CMD_E_BAD_FILE        -5
#define CMD_E_NO_MEM          -6

cmd_t *cmd_find( char *name );

#endif /* __cmd_h__ */
