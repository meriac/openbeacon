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

#ifndef LOGIN_H_
#define LOGIN_H_

enum enUserLoginStatus
{
  WaitForUsername,
  EnterUsername,
  WaitForPassword,
  EnterPassword,
  LoggedIn,
  LoggedOut
};
extern enum enUserLoginStatus UserLoginStatus;

void UserLoginInit (void);
void UserLogin (char Temp);

#endif /*LOGIN_H_ */
