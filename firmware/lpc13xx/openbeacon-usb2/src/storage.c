/***************************************************************
 *
 * OpenBeacon.org - FLASH storage support
 *
 * Copyright 2010 Milosch Meriac <meriac@openbeacon.de>
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
#include "spi.h"

void
storage_status (void)
{
    static const uint8_t cmd_jedec_read_id=0x9F;
    uint8_t rx[3];
    spi_txrx (SPI_CS_FLASH, &cmd_jedec_read_id, sizeof(cmd_jedec_read_id), rx, sizeof(rx));

    /* Show FLASH ID */
    debug_printf(" * FLASH: ID:%02X-%02X-%02X\n",rx[0],rx[1],rx[2]);
}

void
storage_read (uint32_t pos,uint8_t length,uint8_t *data)
{
    uint8_t tx[4];

    tx[0]=0x03; /* 25MHz Read */
    tx[1]=(uint8_t)(pos>>16);
    tx[2]=(uint8_t)(pos>> 8);
    tx[3]=(uint8_t)(pos);

    spi_txrx (SPI_CS_FLASH, tx, sizeof(tx), data, length);
}

void
storage_init (void)
{
  /* setup SPI chipselect pin */
  spi_init_pin (SPI_CS_FLASH);
}
