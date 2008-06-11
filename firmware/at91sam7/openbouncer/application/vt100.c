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

#include "vsprintf.h"
#include "vt100.h"
#include "usbio.h"

/* **********************************************************************
 *	RIS - Reset To Initial State
 *
 *	Reset the VT100 to its initial state, i.e., the state it has after it
 *	is powered on. This also causes the execution of the power-up
 *	self-test and signal INIT H to be asserted briefly.
 ********************************************************************* */
void
vt100Init (void)
{
  // ESC c
  DumpStringToUSB ("\x1B\x63");
}

/* **********************************************************************
 *	ED - Erase In Display
 *
 *	Erase all of the display - all lines are erased, changed to single
 *	width, and the cursor does not move.
 ********************************************************************* */
void
vt100ClearScreen (void)
{
  // ESC [ 2 J
  DumpStringToUSB ("\x1B[2J");
}

/* **********************************************************************
 *	EL - Erase In Line
 *
 *	Erase from the active position to the end of the line, inclusive.
 ********************************************************************* */
void
vt100EraseToLineEnd (void)
{
  // ESC [ 0 K
  DumpStringToUSB ("\x1B[0K");
}

/* **********************************************************************
 *	CUP - Cursor Position
 *
 *  The CUP sequence moves the active position to the position specified
 *	by the parameters. This sequence has two parameter values, the first
 *	specifying the line position and the second specifying the column
 *	position. A parameter value of zero or one for the first or second 
 *	parameter moves the active position to the first line or column in
 *	the display, respectively. The default condition with no parameters
 *	present is equivalent to a cursor to home action. In the VT100, this
 *	control behavesidentically with its format effector counterpart, HVP.
 *	Editor Function
 *
 *	@param  line	Line number
 *	@param  col	Column number
 ********************************************************************* */
void
vt100SetCursorPos (unsigned char line, unsigned char col)
{
  char Text_Buffer[10];

  // ESC [ Pl ; Pc H
  sprintf (Text_Buffer, "\x1B[%d;%dH", line, col);
  DumpStringToUSB (Text_Buffer);
}

/* **********************************************************************
 *	Cursor forward (right)
 *
 *  Cursor Movement Command in the ANSI Compatible Mode.
 ********************************************************************* */
void
vt100MoveCursorRight (void)
{
  // ESC [ VT100_ARROWRIGHT
  DumpStringToUSB ("\x1B[C");
}

/* **********************************************************************
 *	Cursor backward (left)
 *
 *  Cursor Movement Command in the ANSI Compatible Mode.
 ********************************************************************* */
void
vt100MoveCursorLeft (void)
{
  // ESC [ VT100_ARROWLEFT
  DumpStringToUSB ("\x1B[D");
}
