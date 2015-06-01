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
	{13,2, 9}, // 18 remember,
	{16,0, 6}, // 19 should
	{ 8,4, 3}, // 20 the
	{ 8,4, 5}, // 21 their
	{ 0,2, 7}, // 22 treason
	{13,3, 3}, // 23 why
	{ 0,1, 3}, // 24 guy
	{ 7,1, 6}, // 25 fawkes
	{ 3,1, 4}, // 26 1605
	{ 0,0,12}, // 27 governments.
};

static const int g_sentence[] = {
	// Remember, Remember, the Fifth of November,
	18, -1, 18, -1, 20, 4, 13, 12, -1,
	// the Gunpowder Treason and Plot
	20, 7, 22, 1, 15, -1,
	// I know of no reason why the Gunpowder Treason
	8, 9, 13, 10, 16, -1, 23, 20, 7, 22, -1,
	// should ever be forgot
	19, 3, -1, 2, -1, 5, -1, -1,

	// guy fawkes, 1605
	24, 25, -1, 26, -1, -1,

	// People should not be afraid of their governments
	14, 19, 11, 2, 0, 13, 21, 27, -1,
	6, 19, 2, 0, 13, 21, 14, -1, -1,

	// long pause
	-1, -1,
};

#define WORD_COUNT ((int)(sizeof(g_sentence)/sizeof(g_sentence[0])))

#endif/*__WORDS_H__*/
