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

#include "cmd_main.h"
#include "usbio.h"
#include "commandline.h"
#include "vt100.h"
#include "cmd_key.h"
#include "cmd_user.h"
#include "login.h"

/**********************************************************************/

tCmdlineMenu cmdlineMenuMain[] = {
  {"!", cmd_Main_init},
  {"?", cmd_Main_F_help},
  {"help", cmd_Main_F_help},
  {"key", cmd_Key_init},
  {"user", cmd_User_init},
  {"clean", vt100ClearScreen},
  {"logout", cmd_Main_Logout},
};

/**********************************************************************/

void
cmd_Main_init (void)
{
  CmdlineMenuAnzahl = 7;
  strcpy ((char *) (CmdlineMenuPrompt), "main>");
  CmdlineMenuItem = cmdlineMenuMain;
}

/**********************************************************************/

void
cmd_Main_F_help (void)
{
  DumpStringToUSB ("*****************************************************\n\r"
		   "* OpenBouncer USB terminal                          *\n\r"
		   "* (C) 2007 Milosch Meriac <meriac@openbeacon.de>    *\n\r"
		   "* (C) 2007 Angelo Cuccato <cuccato@web.de>          *\n\r"
		   "*****************************************************\n\r"
		   "* Available commands are:\n\r"
		   "*\t!                   - do nothing\n\r"
		   "*\thelp                - displays available commands\n\r"
		   "*\tkey                 - goto key menu\n\r"
		   "*\tuser                - goto key user\n\r"
		   "*\tclean               - display clean\n\r"
		   "*\tlogout              - user logout\n\r"
		   "*****************************************************\n\r");
}

/**********************************************************************/

void
cmd_Main_Logout (void)
{
  UserLoginStatus = LoggedOut;
}
