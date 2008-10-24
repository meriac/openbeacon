#!/usr/bin/env python
#
#  This file is part of the SocioPatterns firmware
#  Copyright (C) 2008 Istituto per l'Interscambio Scientifico I.S.I.
#  You can contact us by email (isi@isi.it) or write to:
#  ISI Foundation, Viale S. Severo 65, 10133 Torino, Italy. 
#
#  This program was written by Ciro Cattuto <ciro.cattuto@gmail.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; version 2.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

import sys, string

FIRMWARE_FILE = 'firmware.hex'
FIRMWARE_OID_FILE = 'firmware_%04d.hex'
oid = int(sys.argv[1])

def checksum(record):
    S = 0
    for i in range(0, len(record), 2):
        S += int(record[i:i+2], 16)
    return 0xff & ((0xff ^ (S & 0xff)) + 1)
        

f = open(FIRMWARE_FILE)
FIRMWARE = f.readlines()
f.close

f = open(FIRMWARE_OID_FILE % oid, 'w')
for line in FIRMWARE[:-3]:
    f.write(line)

eeprom_data = FIRMWARE[-3][:9]
eeprom_data += '%02x00' % (oid & 0xff)
eeprom_data += '%02x00' % (oid >> 8)

seq = (oid * 17) << 7
for i in range(4):
    eeprom_data += '%02x00' % (seq & 0xff)
    seq = seq >> 8
    
eeprom_data += '00000000%02x\n' % checksum(eeprom_data[1:])

f.write(eeprom_data)
f.write(FIRMWARE[-2])
f.write(FIRMWARE[-1])

