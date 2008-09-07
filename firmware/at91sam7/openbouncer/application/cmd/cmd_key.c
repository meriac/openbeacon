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

#include <AT91SAM7.h>
#include <FreeRTOS.h>
#include <string.h>		// include standard C string functions

#include "cmd_key.h"
#include "usbio.h"
#include "commandline.h"
#include "cmd_main.h"
#include "BurgWaechter_TSE_3000/lock.h"

#define SecureKeyLen 16		// 16 Byte = 128 Bit

/**********************************************************************/


tCmdlineMenu cmdlineMenuKey[] = {
  {"!", cmd_Main_init}
  ,
  {"?", cmd_Key_F_help}
  ,
  {"help", cmd_Key_F_help}
  ,
  {"add", cmd_Key_Add}
  ,
  {"del", cmd_Key_Del}
  ,
  {"count", cmd_Key_Count}
  ,
  {"dump", cmd_Key_Dump}
  ,
  {"open", cmd_Key_Open}
};

/**********************************************************************/

void
cmd_Key_init (void)
{
  CmdlineMenuAnzahl = 8;
  strcpy ((char *) (CmdlineMenuPrompt), "key>");
  CmdlineMenuItem = cmdlineMenuKey;
}

/**********************************************************************/

void
cmd_Key_F_help (void)
{
  DumpStringToUSB ("*****************************************************\n\r"
		   "* OpenBouncer USB terminal                          *\n\r"
		   "* (C) 2007 Milosch Meriac <meriac@openbeacon.de>    *\n\r"
		   "*****************************************************\n\r"
		   "* Available commands are:\n\r"
		   "*\t!                   - goto main menu\n\r"
		   "*\thelp                - displays available commands\n\r"
		   "*\tadd <SecureKey>     - add a secure key\n\r"
		   "*\tdel <KeyNr>         - delete Key with <KeyNr>\n\r"
		   "*\tcount               - count keys\n\r"
		   "*\tdump                - dump encrypted SecureKeys\n\r"
		   "*****************************************************\n\r");
}

/**********************************************************************/

void
cmd_Key_Add (void)
{
  unsigned char i;
  char Key[SecureKeyLen + 1];
  bzero (Key, SecureKeyLen + 1);

  // Decode input
  for (i = 0; i < SecureKeyLen; i++)
    Key[i] = cmdlineGetArgHex (i + 1);

  DumpStringToUSB ("*AddKey: \"");
  DumpStringToUSB (Key);
  DumpStringToUSB ("\"\n\r");

  DumpStringToUSB ("*New KeyNr: 4711\n\r");
}

/**********************************************************************/

void
cmd_Key_Del (void)
{
  unsigned int KeyNr;

  KeyNr = cmdlineGetArgInt (1);

  DumpStringToUSB ("*DelKey: \"");
  DumpUIntToUSB (KeyNr);
  DumpStringToUSB ("\"\n\r");
}

/**********************************************************************/

void
cmd_Key_Count (void)
{
  DumpStringToUSB ("*KeyCount=23\n\r");
}

/**********************************************************************/

void
cmd_Key_Dump (void)
{
  DumpStringToUSB ("*dump...#\n\r");
}

/**********************************************************************/

void
cmd_Key_Open (void)
{
  DumpStringToUSB ("open... ");
  if (vOpenLock ())
    DumpStringToUSB ("ok\n\r");
  else
    DumpStringToUSB ("not ok\n\r");
}
