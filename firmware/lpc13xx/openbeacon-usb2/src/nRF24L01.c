/***************************************************************
 *
 * OpenBeacon.org - nRF24L01 2.4GHz active RFID support
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
#include "nRF24L01.h"
#include "spi.h"

/* IO definitions */
#define RF_IRQ_CPU_PORT 0
#define RF_IRQ_CPU_PIN 4
#define CPU_CE_RF_PORT 0
#define CPU_CE_RF_PIN 4
#define CPU_SWITCH_RF_PORT 3
#define CPU_SWITCH_RF_PIN 2

void
nrf_init (void)
{
  /* setup SPI chipselect pin */
  spi_init_pin (SPI_CS_NRF);

  /* setup IOs */
  GPIOSetDir (RF_IRQ_CPU_PORT, RF_IRQ_CPU_PIN, 0);
  GPIOSetDir (CPU_CE_RF_PORT, CPU_CE_RF_PIN, 1);
  GPIOSetValue (CPU_CE_RF_PORT, CPU_CE_RF_PIN, 0);
  GPIOSetDir (CPU_SWITCH_RF_PORT, CPU_SWITCH_RF_PIN, 1);
  GPIOSetValue (CPU_SWITCH_RF_PORT, CPU_SWITCH_RF_PIN, 0);
}
