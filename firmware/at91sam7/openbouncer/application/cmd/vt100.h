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

#ifndef VT100_H_
#define VT100_H_

// Besondere Zeichen
#define ASCII_BEL				0x07
#define ASCII_BS				0x08
#define ASCII_TAB				0x09
#define ASCII_CR				0x0D
#define ASCII_LF				0x0A
#define ASCII_ESC				0x1B
#define ASCII_DEL				0x7F
#define ASCII_POS1				0x32
#define ASCII_END				0x35

// Pfeiltasten
#define VT100_ARROWUP			'A'	// 41 Hex
#define VT100_ARROWDOWN			'B'	// 42 Hex
#define VT100_ARROWRIGHT		'C'	// 43 Hex
#define VT100_ARROWLEFT			'D'	// 44 Hex
#define VT100_POS1				'H'	// 48 Hex
#define VT100_END				'K'	// 4B Hex


// constants/macros/typdefs
// text attributes
#define VT100_ATTR_OFF			0
#define VT100_BOLD				1
#define VT100_USCORE			4
#define VT100_BLINK				5
#define VT100_REVERSE			7
#define VT100_BOLD_OFF			21
#define VT100_USCORE_OFF		24
#define VT100_BLINK_OFF			25
#define VT100_REVERSE_OFF		27


// functions
void vt100Init (void);
void vt100ClearScreen (void);
void vt100EraseToLineEnd (void);
void vt100SetAttr (unsigned char attr);
void vt100SetCursorMode (unsigned char visible);
void vt100SetCursorPos (unsigned char line, unsigned char col);
void vt100MoveCursorRight (void);
void vt100MoveCursorLeft (void);

#endif /*VT100_H_ */
