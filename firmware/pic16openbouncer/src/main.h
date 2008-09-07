/***************************************************************
 *
 * OpenBeacon.org - definitions nRF opcode chunk
 *
 * Copyright 2006 Milosch Meriac <meriac@openbeacon.de>
 *
/***************************************************************

/*
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

#ifndef __OPENBEACON_MAIN_H__
#define __OPENBEACON_MAIN_H__

#include "openbouncer.h"

typedef struct
{
  u_int8_t size, opcode;
  TBouncerEnvelope cmd;
  u_int8_t eot;
} TMacroBeacon;

extern TMacroBeacon g_MacroBeacon;

#endif/*__OPENBEACON_MAIN_H__*/
