/***************************************************************
 *
 * OpenBeacon.org - network code
 *
 * Copyright 2010 Milosch Meriac <meriac@openbeacon.de>
 *
 * basically starts the USB task, initializes all IO ports
 * and introduces idle application hook to handle the HF traffic
 * from the nRF24L01 chip
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

#ifndef __NETWORK_H__
#define __NETWORK_H__

extern void vNetworkInit (void);
extern void vNetworkSendBeaconToServer (void);
extern void vNetworkDumpConfig (void);
extern void vNetworkResetDefaultSettings (void);
extern int vNetworkSetIP (struct ip_addr *ip, const char *ip_string,
						  const char *ip_class);

#endif/*__NETWORK_H__*/
