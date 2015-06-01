/***************************************************************
 *
 * OpenBeacon.org - RGB Strip control - word database
 *
 * Copyright 2015 Milosch Meriac <milosch@meriac.com>
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

#ifndef __WORDS_H__
#define __WORDS_H__

#define LED_X (22L)
#define LED_Y (5L)
#define LED_COUNT (LED_X*LED_Y)


/* People should not be afraid of their governments.
 * Governments should be afraid of their people. */

/* Remember, the Fifth of November, the Gunpowder Treason and Plot.
 * I know of no reason why the Gunpowder Treason should ever be forgot*/

typedef struct {
	int x,y, length;
} TWordPos;

static const TWordPos g_words[] = {
	{13,4, 6}, //  0 afraid 
	{19,4, 3}, //  1 and
	{18,2, 2}, //  2 be
	{ 6,3, 4}, //  3 ever
	{ 1,3, 5}, //  4 fifth
	{16,3, 6}, //  5 forgot
	{ 0,0,11}, //  6 governments
	{13,1, 9}, //  7 gunpowder
	{ 2,3, 1}, //  8 i 
	{10,3, 4}, //  9 know
	{11,3, 2}, // 10 no
	{ 6,4, 3}, // 11 not
	{ 6,2, 8}, // 12 november
	{ 0,3, 2}, // 13 of
	{ 0,4, 6}, // 14 people
	{12,0, 4}, // 15 plot
	{ 1,2, 6}, // 16 reason
	{13,2, 8}, // 17 remember
	{16,0, 6}, // 18 should
	{ 8,4, 3}, // 19 the
	{ 8,4, 5}, // 20 their
	{ 0,2, 7}, // 21 treason
	{13,3, 3}  // 22 why
};

#define WORD_COUNT ((int)(sizeof(g_words)/sizeof(g_words[0])))

#endif/*__WORDS_H__*/
