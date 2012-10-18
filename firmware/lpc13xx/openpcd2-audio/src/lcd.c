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
#include "lcd.h"

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
#if 0
	const uint8_t cas_pkt[3] = {LCDCMD_CASET,1,CHARACTER_SIZE};
	const uint8_t ras_pkt[3] = {LCDCMD_PASET,1,CHARACTER_SIZE};

	lcd_send (cas_pkt, sizeof(cas_pkt));
	lcd_send (ras_pkt, sizeof(ras_pkt));
#endif
}

void
lcd_update_char(char chr)
{
	(void)chr;
#if 0
	int i;
	const uint8_t *c;
	uint8_t pkt[3],data,count;

	if( (chr<CHARACTER_START) && (chr>=(CHARACTER_START+CHARACTER_COUNT)) )
		return;
	c=font[chr-CHARACTER_START];

	lcd_update_fullscreen();

	pkt[0]=LCDCMD_RAMWR;
	spi_txrx ( SPI_CS_LCD|SPI_CS_MODE_LCD_CMD|SPI_CS_MODE_SKIP_CS_DEASSERT, pkt, 1, NULL, 0);

	i=0;
	while(i<((CHARACTER_SIZE*CHARACTER_SIZE)/2))
	{
		data = *c++;
		if(data==0xFF || data==0x00)
		{
			count = *c++;
			if(!count)
				break;
		}
		else
			count = 1;

		i+=count;

		while(count--)
		{
			pkt[0] = (data>>4)|(data&0xF0);
			pkt[1] = data;
			pkt[2] = (data<<4)|(data&0x0F);

			spi_txrx ( SPI_CS_LCD|SPI_CS_MODE_LCD_DAT|SPI_CS_MODE_SKIP_CS_ASSERT|SPI_CS_MODE_SKIP_CS_DEASSERT, pkt, sizeof(pkt), NULL, 0);
		}
	}

	spi_txrx_done ( SPI_CS_LCD );
#endif
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
