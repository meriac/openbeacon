/***************************************************************
 *
 * OpenBeacon.org - LPC13xx LCD Display Routines
 *
 * Copyright 2012 Milosch Meriac <meriac@openbeacon.de>
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

#ifndef __LCD_H__
#define __LCD_H__

#define LCDCMD_SLEEPOUT		0x11
#define LCDCMD_INVON		0x20
#define LCDCMD_SETCON		0x25
#define LCDCMD_DISPON		0x29
#define LCDCMD_CASET		0x2A
#define LCDCMD_PASET		0x2B
#define LCDCMD_RAMWR		0x2C
#define LCDCMD_COLMOD		0x3A

extern void lcd_init (void);
extern void lcd_brightness (uint8_t percent);
extern void lcd_cmd (uint8_t cmd);
extern void lcd_cmddat (uint8_t cmd, uint8_t data);
extern void lcd_update_char(char chr);
extern void lcd_update_screen(uint16_t id);


#endif/*__LCD_H__*/
