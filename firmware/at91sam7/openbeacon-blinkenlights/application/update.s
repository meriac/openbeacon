/***************************************************************
 *
 * OpenBeacon.org - ATMEL Original Boot Loader Backup
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

.global bootloader
.global bootloader_orig
.global bootloader_orig_end

.section .bootloader,"ax"
         .code 32
         .align 0

bootloader:
	 .incbin "application/update/AT91SAM7S64-bootloader.bin"

.section .rodata
         .data 32
         .align 0

bootloader_orig:
	 .incbin "application/update/AT91SAM7S64-bootloader-orig.bin"
bootloader_orig_end:
