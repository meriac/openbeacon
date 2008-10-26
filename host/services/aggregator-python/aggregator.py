#!/usr/bin/env python
#
#  OpenBeacon Python aggregator
#
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

import sys, string, time
from twisted.application import internet, service
from twisted.internet import protocol
from twisted.python import log
import struct
from socket import inet_aton, inet_ntoa

# server ports
UDP_PORT = 2342

# decrypt flag
OPENBEACON_DECODE = 1
OPENBEACON_DECODE_PUREPYTHON = 0

# OpenBeacon protocols
PROTO_BEACONTRACKER = 23

# XXTEA key
TEA_CRYPTO_KEY = ( 0xDEADBEEF, 0xDEADBEEF, 0xDEADBEEF, 0xDEADBEEF )


# ---------------------------------------------------------------------------

class Sighting:
    def __init__(self, tstamp, ip, id, seq, strength, flags):
        self.t = tstamp
        self.ip = ip
        self.id = id
        self.seq = seq
        self.strength = strength
        self.flags = flags

    def __repr__(self):
       return '[sighting: %ld %s %d 0x%08x %d %d]' % (self.t, inet_ntoa(struct.pack('>L', self.ip)), self.id, self.seq, self.strength, self.flags)

class OpenBeaconSystem:
    def __init__(self):
        self.max_time = -1

    def process_sighting(self, s):
        self.max_time = s.t

        # do your stuff
        print s

class Receiver(protocol.DatagramProtocol):
    def __init__(self, process_sighting):
        self.process_sighting = process_sighting

    def datagramReceived(self, data, (ip, port)):
        tstamp = int(time.time())
        station_id = struct.unpack('>L', inet_aton(ip))[0]

        if OPENBEACON_DECODE:
            data = xxtea.decode(data)

        (length, proto) = struct.unpack("!BB", data[:2])
#        log.msg('got UDP packet from %s: proto=%d' % (ip, proto))

        if proto == PROTO_BEACONTRACKER:
            (length, proto, flags, strength, seq, id, reserved, crc) = struct.unpack("!BBBBLLHH", data)
            if crc != xxtea.crc16(data[:14]):
                log.msg('Rejecting packet from 0x%08x on CRC' % station_id)
                return

            s = Sighting(tstamp, station_id, id, seq, strength, flags)
            self.process_sighting(s)



# ---------------------------------------------------------------------------

if not OPENBEACON_DECODE_PUREPYTHON:
    try:
        import xxtea
    except ImportError:
        OPENBEACON_DECODE_PUREPYTHON = 1
        print 'Error loading XXTEA module. Falling back to (slow!) pure Python decoding.'

if OPENBEACON_DECODE_PUREPYTHON:
    import xxtea_python as xxtea

apply(xxtea.set_key, TEA_CRYPTO_KEY)

# -----------------------------------------------------------------------------

OBS = OpenBeaconSystem()

application = service.Application('OpenBeacon On-Line Post-Processing System')

receiver = Receiver(OBS.process_sighting)
udpServer = internet.UDPServer(UDP_PORT, receiver)
udpServer.setServiceParent(application)
log.msg('Asynchronous OpenBeacon aggregator server listening on UDP port %d' % UDP_PORT)


