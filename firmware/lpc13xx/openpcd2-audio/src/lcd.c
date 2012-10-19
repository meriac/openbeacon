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
#include <openbeacon.h>
#include "storage.h"
#include "beacon_db.h"
#include "lcd.h"

#define CHARACTER_SIZE 130

void
lcd_send(const uint8_t *pkt, uint16_t len)
{
	spi_txrx ( SPI_CS_LCD|SPI_CS_MODE_LCD_CMD|SPI_CS_MODE_LCD_DAT, pkt, len, NULL, 0);
}

void
lcd_cmd (uint8_t cmd)
{
	spi_txrx ( SPI_CS_LCD|SPI_CS_MODE_LCD_CMD, &cmd, sizeof(cmd), NULL, 0);
}

void
lcd_cmddat (uint8_t cmd, uint8_t data)
{
	uint8_t pkt[2];

	/* assemble command packet with one parameter */
	pkt[0] = cmd;
	pkt[1] = data;

	spi_txrx ( SPI_CS_LCD|SPI_CS_MODE_LCD_CMD|SPI_CS_MODE_LCD_DAT, &pkt, sizeof(pkt), NULL, 0);
}

void
lcd_update_fullscreen(void)
{
	const uint8_t cas_pkt[3] = {LCDCMD_CASET,1,CHARACTER_SIZE};
	const uint8_t ras_pkt[3] = {LCDCMD_PASET,1,CHARACTER_SIZE};

	lcd_send (cas_pkt, sizeof(cas_pkt));
	lcd_send (ras_pkt, sizeof(ras_pkt));
}

void
lcd_update_screen(uint16_t id)
{
	int length, i, read;
	uint8_t pkt[3],buffer[32],data,count,*c;

	if((length=storage_db_find (DB_DIR_ENTRY_TYPE_VIDEO_GREY,id))>=0)
		debug_printf("VIDEO: show id=%i with %i bytes\n",id,length);
	else
	{
		debug_printf("VIDEO ERROR: Can't find video ID %i in database\n",id);
		return;
	}

	lcd_update_fullscreen();

	i=length=read=0;
	c=NULL;
	while((length=storage_db_read(buffer,sizeof(buffer)))>0)
	{
		c=buffer;
		read+=length;

		while(length--)
		{
			data = *c++;
			if(data==0xFF || data==0x00)
			{
				if(!length)
				{
					length=storage_db_read(buffer,sizeof(buffer));
					if(!length)
					{
						debug_printf("VIDEO ERROR: Underflow\n");
						break;
					}
					c=buffer;
					read+=length;
				}

				length--;
				count = *c++;
				if(!count)
				{
					debug_printf("VIDEO: Terminated\n");
					break;
				}
			}
			else
				count = 1;

			i+=count;

			pkt[0] = LCDCMD_RAMWR;
			spi_txrx ( SPI_CS_LCD|SPI_CS_MODE_LCD_CMD|SPI_CS_MODE_SKIP_CS_DEASSERT, pkt, 1, NULL, 0);

			while(count--)
			{
				pkt[0] = (data>>4)|(data&0xF0);
				pkt[1] = data;
				pkt[2] = (data<<4)|(data&0x0F);
				spi_txrx ( SPI_CS_LCD|SPI_CS_MODE_LCD_DAT|SPI_CS_MODE_SKIP_CS_ASSERT|SPI_CS_MODE_SKIP_CS_DEASSERT, pkt, sizeof(pkt), NULL, 0);
			}

			spi_txrx_done ( SPI_CS_LCD );
		}
	}
	debug_printf("VIDEO: displayed %i bytes (read %i bytes)\n",i,read);
}

void
lcd_brightness (uint8_t percent)
{
	GPIOSetValue(LCD_PWRPWM_PORT, LCD_PWRPWM_PIN, percent ? 1:0);
}

void
lcd_init (void)
{
	/* Set LED reset pin to output */
	GPIOSetDir (LCD_RESET_PORT, LCD_RESET_PIN, 1);
	GPIOSetValue(LCD_RESET_PORT, LCD_RESET_PIN, 0);

	/* Set LED background pin to output */
	GPIOSetDir (LCD_PWRPWM_PORT, LCD_PWRPWM_PIN, 1);
	GPIOSetValue(LCD_PWRPWM_PORT, LCD_PWRPWM_PIN, 0);

	/* setup SPI chipselect pin */
	spi_init_pin (SPI_CS_LCD);

	/* release reset pin */
	pmu_wait_ms (500);
	GPIOSetValue(LCD_RESET_PORT, LCD_RESET_PIN, 1);

	/* init LCD display */
	lcd_cmd (LCDCMD_SLEEPOUT);
	lcd_cmd (LCDCMD_INVON);
	lcd_cmddat (LCDCMD_SETCON, 0x30);
	lcd_cmd (LCDCMD_DISPON);

	/* enable background light */
	GPIOSetValue(LCD_PWRPWM_PORT, LCD_PWRPWM_PIN, 1);
}
