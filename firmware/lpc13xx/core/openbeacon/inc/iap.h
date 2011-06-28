/***************************************************************
 *
 * OpenBeacon.org - IAP - in application programming
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

#ifndef __IAP_H__
#define __IAP_H__

#define DEVICE_UID_MEMBERS 4
typedef uint32_t TDeviceUID[DEVICE_UID_MEMBERS];

extern void iap_read_uid (TDeviceUID * uid);
extern void iap_invoke_isp (void);

#endif/*__IAP_H__*/
