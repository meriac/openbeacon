/* AT91SAM7 debug function implementation for OpenPCD
 * (C) 2006 by Harald Welte <hwelte@hmw-consulting.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by 
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <board.h>
#include "dbgu.h"
#ifdef  AT91C_DBGU_BAUD

void AT91F_DBGU_Init(void)
{
	/* Open PIO for DBGU */
	AT91F_DBGU_CfgPIO();
	/* Enable Transmitter & receivier */
	((AT91PS_USART) AT91C_BASE_DBGU)->US_CR = 
					AT91C_US_RSTTX | AT91C_US_RSTRX;

	/* Configure DBGU */
	AT91F_US_Configure((AT91PS_USART)AT91C_BASE_DBGU,
			   MCK, AT91C_US_ASYNC_MODE,
			   AT91C_DBGU_BAUD, 0);

	/* Enable Transmitter & receivier */
	((AT91PS_USART) AT91C_BASE_DBGU)->US_CR = 
					AT91C_US_RXEN | AT91C_US_TXEN;
}

void AT91F_DBGU_Frame(char *buffer, int len)
{
    if(len>0)
	AT91F_US_SendFrame((AT91PS_USART) AT91C_BASE_DBGU, (unsigned char *)buffer, len, 0, 0);
}

#endif/*AT91C_DBGU_BAUD*/
