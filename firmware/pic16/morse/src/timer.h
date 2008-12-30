/***************************************************************
 *
 * OpenBeacon.org - definition exported timer function
 *
 * Copyright 2006 Harald Welte <laforge@openbeacon.org>
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

#ifndef _TIMER_H
#define _TIMER_H

#define TIMER1_HZ	32768
#define TIMER1_JIFFIES_PER_MS 33

void timer1_init (void);
void sleep_jiffies (unsigned short jiffies);

#endif /* _TIMER_H */
