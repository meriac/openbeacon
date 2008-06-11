/***************************************************************
 *
 * OpenBeacon.org - OpenBeacon link layer protocol
 *
 * Copyright 2007 Angelo Cuccato <cuccato@web.de>
 *
 ***************************************************************

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef COMMANDLINE_H_
#define COMMANDLINE_H_

#define CMDLINE_MAX_CMD_LENGTH		30
// Maximale commandline buffer size

typedef void (*CmdlineFuncPtrType) (void);

typedef struct
{
  char Command[CMDLINE_MAX_CMD_LENGTH];
  CmdlineFuncPtrType Function;
} tCmdlineMenu;


extern tCmdlineMenu *CmdlineMenuItem;
extern unsigned char CmdlineMenuAnzahl;
extern char CmdlineMenuPrompt[10];

void cmdlineInit (void);
void cmdlineDecodeInput (char c);
void cmdlineRepaint (void);

char *cmdlineGetArgStr (unsigned char argnum);
long cmdlineGetArgInt (unsigned char argnum);
long cmdlineGetArgHex (unsigned char argnum);

#endif /*COMMANDLINE_H_ */
