/*
 
Public platform independent Near Field Communication (NFC) library
Copyright (C) 2009, Roel Verdult
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef _LIBNFC_DEV_ACR122_H_
#define _LIBNFC_DEV_ACR122_H_

#include "defines.h"
#include "types.h"

// Various additional features this device supports
char* dev_acr122_firmware(const dev_spec ds);
bool dev_acr122_led_red(const dev_spec ds, bool bOn);

extern const struct libnfc_driver_info ACR122_DRIVER_INFO;

#endif // _LIBNFC_DEV_ACR122_H_

