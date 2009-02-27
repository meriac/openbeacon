/***************************************************************
 *
 * OpenBeacon.org - USB boot loader console handling
 *
 * Copyright 2009 Milosch Meriac <meriac@bitmanufaktur.de>
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

#ifndef __CONSOLE_H__
#define __CONSOLE_H__

extern void vInitConsole (void);
extern int puts (const char* message);

#endif/*__CONSOLE_H__*/
