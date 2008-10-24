/*
    This file is part of the SocioPatterns firmware
    Copyright (C) 2008 Istituto per l'Interscambio Scientifico I.S.I.
    You can contact us by email (isi@isi.it) or write to:
    ISI Foundation, Viale S. Severo 65, 10133 Torino, Italy. 

    This program was written by Ciro Cattuto <ciro.cattuto@gmail.com>

    This program is based on:
    OpenBeacon.org - definition exported timer function
    Copyright 2006 Harald Welte <laforge@openbeacon.org>

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

#ifndef _TIMER_H
#define _TIMER_H

#define TIMER1_HZ	32768
#define TIMER1_JIFFIES_PER_MS 33

void timer1_init (void);
unsigned short timer1_get (void);
unsigned short sleep_jiffies (unsigned short jiffies);

#endif /* _TIMER_H */
