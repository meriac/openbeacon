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


#include "cmd_user.h"
#include "usbio.h"
#include "commandline.h"
#include "cmd_main.h"

/**********************************************************************/

tCmdlineMenu cmdlineMenuUser[] = {
  {"!", cmd_Main_init},
  {"?", cmd_User_F_help},
  {"help", cmd_User_F_help},
  {"add", cmd_User_Add},
  {"del", cmd_User_Del},
  {"setpasswd", cmd_User_SetPassword},
  {"setright", cmd_User_SetRight},
  {"getright", cmd_User_GetRight},
  {"list", cmd_User_List}
};

/**********************************************************************/

void
cmd_User_init (void)
{
  CmdlineMenuAnzahl = 9;
  strcpy ((char *) (CmdlineMenuPrompt), "user>");
  CmdlineMenuItem = cmdlineMenuUser;
}

/**********************************************************************/

void
cmd_User_F_help (void)
{
  DumpStringToUSB ("*****************************************************\n\r"
		   "* OpenBouncer USB terminal                          *\n\r"
		   "* (C) 2007 Milosch Meriac <meriac@openbeacon.de>    *\n\r"
		   "*****************************************************\n\r"
		   "* Available commands are:\n\r"
		   "*\t!                           - goto main menu\n\r"
		   "*\thelp                        - displays available commands\n\r"
		   "*\tadd <username>              - add user\n\r"
		   "*\tdel <username>              - del user\n\r"
		   "*\tsetpasswd <username>        - set user passowrd\n\r"
		   "*\tsetright <username> <right> - set user rights\n\r"
		   "*\tgetright <username>         - show user rights\n\r"
		   "*\tlist                        - list user with rights\n\r"
		   "*****************************************************\n\r");
}

/**********************************************************************/

void
cmd_User_Add (void)
{
}

/**********************************************************************/

void
cmd_User_Del (void)
{
}

/**********************************************************************/

void
cmd_User_SetPassword (void)
{
}

/**********************************************************************/

void
cmd_User_SetRight (void)
{
}

/**********************************************************************/

void
cmd_User_GetRight (void)
{
}

/**********************************************************************/

void
cmd_User_List (void)
{
}
