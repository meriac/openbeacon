/***************************************************************
 *
 * OpenBeacon.org - OpenBeacon UART driver
 *
 * Copyright 2007 Milosch Meriac <meriac@openbeacon.de>
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
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <string.h>
#include <board.h>

static void
vnRFtaskUart (void *parameter)
{
    (void) parameter;

    AT91F_PIO_SetOutput(WLAN_PIO, WLAN_RESET);

    for(;;)
    {
	vTaskDelay(1000/portTICK_RATE_MS);
    }
}

void
uart_init (void)
{
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_US0);
    AT91F_PIO_CfgPeriph(WLAN_PIO, (WLAN_RXD|WLAN_TXD|WLAN_RTS|WLAN_CTS), 0);

    AT91F_PIO_CfgOutput(WLAN_PIO, WLAN_ADHOC|WLAN_RESET|WLAN_WAKE);
    AT91F_PIO_ClearOutput(WLAN_PIO, WLAN_RESET|WLAN_ADHOC|WLAN_WAKE);

    xTaskCreate (vnRFtaskUart, (signed portCHAR *) "vnRFtaskUart", TASK_NRF_STACK,
	       NULL, TASK_NRF_PRIORITY, NULL);
}
