#!/usr/bin/env python
#
#  pure Python implementation of XXTEA decoder and CRC16
#
#  Copyright (C) 2008 Istituto per l'Interscambio Scientifico I.S.I.
#  You can contact us by email (isi@isi.it) or write to:
#  ISI Foundation, Viale S. Severo 65, 10133 Torino, Italy. 
#
#  This program was written by Ciro Cattuto <ciro.cattuto@gmail.com>
#
#  Based on "Correction to xtea" by David J. Wheeler and Roger M. Needham.
#  Computer Laboratory, Cambridge University (1998)
#  http://www.movable-type.co.uk/scripts/xxtea.pdf
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

from socket import htonl, ntohl
import numpy

TEA_KEY = ( 0xDEADBEEF, 0xDEADBEEF, 0xDEADBEEF, 0xDEADBEEF )

def set_key(tea_key1, tea_key2, tea_key3, tea_key4):
    global TEA_KEY
    TEA_KEY = (tea_key1, tea_key2, tea_key3, tea_key4)
 
def decode(data):
    if len(data) % 4:
        raise ValueError, 'argument byte length must be a multiple of 4'

    v = numpy.fromstring(data, dtype=numpy.uint32)
    length = len(v)
    if length < 2:
        raise ValueError, 'argument byte length at least 8 bytes'

    for i in range(length):
        v[i] = ntohl(int(v[i]))

    mask = 0xFFFFFFFFL
    tea_delta = 0x9E3779B9L
    sum = numpy.uint32((6 + 52 / length) * tea_delta) 
    y = v[0]
    while (sum):
        e = (sum >> 2) & 3
        for p in range(length-1, 0, -1):
            z = v[p-1]
            v[p] -= (((z>>5)^(y<<2))+((y>>3)^(z<<4)))^((sum^y)+(TEA_KEY[(p&3)^e]^z)) 
            y = v[p]
        z = v[length-1]
        v[0] -= (((z>>5)^(y<<2))+((y>>3)^(z<<4)))^((sum^y)+(TEA_KEY[(0&3)^e]^z)) 
        y = v[0]
        sum = (sum - tea_delta) & mask

    for i in range(length):
        v[i] = htonl(long(v[i]))

    return v.data

def crc16(data):
    v = numpy.fromstring(data, dtype=numpy.uint8)
    mask = 0xFFFF
    crc = 0xFFFF
    for x in v:
        crc = (crc >> 8) | ((crc << 8) & mask)
        crc = crc ^ x
        crc = crc ^ ((crc & 0xff) >> 4)
        crc = crc ^ ((crc << 12) & mask)
        crc = crc ^ (((crc & 0xff) << 5) & mask)
    return crc

