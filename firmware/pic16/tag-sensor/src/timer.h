/***************************************************************
 *
 * OpenBeacon.org - definition exported timer function
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

#ifndef _TIMER_H
#define _TIMER_H

#define TIMER1_HZ	32768
#define JIFFIES_PER_MS(x) ((x*TIMER1_HZ)/1000)

#define SLEEPDEEP_1ms 0
#define SLEEPDEEP_2ms 1
#define SLEEPDEEP_4ms 2
#define SLEEPDEEP_8ms 3
#define SLEEPDEEP_16ms 4
#define SLEEPDEEP_33ms 5
#define SLEEPDEEP_66ms 6
#define SLEEPDEEP_132ms 7
#define SLEEPDEEP_264ms 8
#define SLEEPDEEP_528ms 9
#define SLEEPDEEP_1057ms 10
#define SLEEPDEEP_2114ms 11

void timer_init (void);
void sleep_2ms (void);
void sleep_deep (unsigned char div);
void sleep_jiffies (unsigned short jiffies);

#endif /* _TIMER_H */
