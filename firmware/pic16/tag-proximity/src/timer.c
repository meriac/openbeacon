/*
    This file is part of the SocioPatterns firmware
    Copyright (C) 2008 Istituto per l'Interscambio Scientifico I.S.I.
    You can contact us by email (isi@isi.it) or write to:
    ISI Foundation, Viale S. Severo 65, 10133 Torino, Italy. 

    This program was written by Ciro Cattuto <ciro.cattuto@gmail.com>

    This program is based on:
    OpenBeacon.org - accurate low power sleep function,
                     based on 32.768kHz watch crystal
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

#include <htc.h>
#include "config.h"
#include "timer.h"

#define	T1CON_TMR1ON 	(1 << 0)
#define	T1CON_TMR1CS	(1 << 1)
#define T1CON_T1SYNC	(1 << 2)
#define T1CON_T1OSCEN	(1 << 3)
#define T1CON_T1CKPS0	(1 << 4)
#define T1CON_T1CKPS1	(1 << 5)
#define T1CON_TMR1GE	(1 << 6)
#define T1CON_T1GINV	(1 << 7)

static char timer1_wrapped;

void
timer1_init (void)
{
  /* Configure Timer 1 to use external 32768Hz crystal and 
   * no (1:1) prescaler*/
  T1CON = T1CON_T1OSCEN | T1CON_T1SYNC | T1CON_TMR1CS;
  timer1_wrapped = 0;
}

void
timer1_set (unsigned short tout)
{
  tout = 0xffff - tout;

  TMR1H = tout >> 8;
  TMR1L = tout & 0xff;
}

void
timer1_go (void)
{
  TMR1IE = 1;
  PEIE = 1;
  INTE = 1;
  GIE = 1;
  TMR1ON = 1;
}

void
timer1_sleep (void)
{
  timer1_wrapped = 0;
  while (timer1_wrapped == 0)
    {
      /* enable peripheral interrupts */
      TMR1IE = 1;
      PEIE = 1;
      INTE = 1; 
      GIE = 1;

      SLEEP ();
    }
}

unsigned short
sleep_jiffies (unsigned short jiffies)
{
  timer1_set (jiffies);
  timer1_go ();
  timer1_sleep ();
  return 0xffff - (TMR1H << 8 | TMR1L);
}

void interrupt
irq (void)
{
  /* timer1 has overflowed */
  if (TMR1IF || INTF)
    {
      timer1_wrapped = 1;

      if (TMR1IF)
        TMR1IF = 0;

      if (INTF)
        INTF = 0;
        
      TMR1ON = 0;
      TMR1IE = 0;
      INTE = 0;
    }
}
