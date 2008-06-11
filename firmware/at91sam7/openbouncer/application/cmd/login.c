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
#include <string.h>

#include "login.h"
#include "commandline.h"
#include "usbio.h"
/**********************************************************************/

#define InputBufferLen	21

enum enUserLoginStatus UserLoginStatus;
char InputBufferUser[InputBufferLen];
char InputBufferPass[InputBufferLen];
unsigned char InputBufferUserPos;
unsigned char InputBufferPassPos;
/**********************************************************************/

void
UserLoginInit (void)
{
  UserLoginStatus = WaitForUsername;
}

/**********************************************************************/

void
UserLogin (char Temp)
{
  if (UserLoginStatus == WaitForUsername)
    {
      InputBufferUserPos = 0;
      UserLoginStatus = EnterUsername;
      Temp = '\r';
      bzero (InputBufferUser, InputBufferLen);
      bzero (InputBufferPass, InputBufferLen);
    }
  if (UserLoginStatus == EnterUsername)
    {
      if (Temp == '\r')
	{
	  if (InputBufferUserPos == 0)
	    DumpStringToUSB ("\n\rUsername>");
	  else
	    UserLoginStatus = WaitForPassword;
	}
      else if (InputBufferUserPos < InputBufferLen)
	{
	  InputBufferUser[InputBufferUserPos++] = Temp;
	  vSendByte (Temp);
	}
    }
  if (UserLoginStatus == WaitForPassword)
    {
      InputBufferPassPos = 0;
      UserLoginStatus = EnterPassword;
      Temp = '\r';
    }
  if (UserLoginStatus == EnterPassword)
    {
      if (Temp == '\r')
	{
	  if (InputBufferPassPos == 0)
	    DumpStringToUSB ("\n\rPassword>");
	  else
	    {
	      /*todo: check user dada */
	      if (InputBufferUser[0] == 'r' && InputBufferPass[0] == 'r')
		{
		  /*todo:  read/set User rights */
		  UserLoginStatus = LoggedIn;
		}
	      else
		UserLoginStatus = LoggedOut;
	    }
	}
      else if (InputBufferPassPos < InputBufferLen)
	{
	  InputBufferPass[InputBufferPassPos++] = Temp;
	  vSendByte (Temp);
	}
    }
  if (UserLoginStatus == LoggedIn)
    {
      DumpStringToUSB ("\n\r\n\r\n\rWelcome!\n\r\n\r");
      cmdlineRepaint ();
    }
  if (UserLoginStatus == LoggedOut)
    {
      DumpStringToUSB ("\n\r\n\r\n\rGood Bye, have a nive day!\n\r\n\r");
      UserLoginStatus = WaitForUsername;
    }
}
