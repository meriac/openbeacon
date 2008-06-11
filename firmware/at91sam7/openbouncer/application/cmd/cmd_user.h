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

#ifndef CMD_USER_H_
#define CMD_USER_H_

#define DefUsernameLen  21

typedef struct
{
  char username[DefUsernameLen];	// 
  char password[16 + 1];	// 128Bit hash + EOL
  unsigned long rights;
} TuserDB;

void cmd_User_init (void);
void cmd_User_F_help (void);
void cmd_User_Add (void);
void cmd_User_Del (void);
void cmd_User_SetPassword (void);
void cmd_User_SetRight (void);
void cmd_User_GetRight (void);
void cmd_User_List (void);

#endif /*CMD_USER_H_ */
