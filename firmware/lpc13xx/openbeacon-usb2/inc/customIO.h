/***************************************************************
 *
 * OpenBeacon.org - custom printf-to-buffer IO redirection
 *
 * Copyright 2010 Milosch Meriac <meriac@openbeacon.de>
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

#ifndef __CUSTOMIO_H__
#define __CUSTOMIO_H__

/* print formatted string into buffer, return string size without termination */
extern uint32_t cIO_snprintf (char *str, uint32_t size, const char *fmt, ...);

#endif/*__CUSTOMIO_H__*/