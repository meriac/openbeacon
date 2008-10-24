/*
    This file is part of the SocioPatterns firmware
    Copyright (C) 2008 Istituto per l'Interscambio Scientifico I.S.I.
	
    You can contact us by email (isi@isi.it) or write to:
    ISI Foundation, Viale S. Severo 65, 10133 Torino, Italy. 

    This program was written by Ciro Cattuto <ciro.cattuto@gmail.com>

    This program is based on:
    OpenBeacon.org - definitions nRF opcode chunk
    Copyright 2006 Milosch Meriac <meriac@openbeacon.de>

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

    This file was modified on: July 1st, 2008
*/

#ifndef __OPENBEACON_MAIN_H__
#define __OPENBEACON_MAIN_H__

#include "openbeacon.h"

bank1 typedef struct
{
  u_int8_t size1, opcode1, rf_setup;
  u_int8_t size2, opcode2, rf_ch;
  u_int8_t size3, opcode3, rf_tx_addr[5];
  u_int8_t size4, opcode4;
  TBeaconEnvelope env;
  u_int8_t eot;
} TMacroBeacon;

extern TMacroBeacon g_MacroBeacon;

#endif /*__OPENBEACON_MAIN_H__*/
