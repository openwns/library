###############################################################################
# This file is part of openWNS (open Wireless Network Simulator)
# _____________________________________________________________________________
#
# Copyright (C) 2004-2007
# Chair of Communication Networks (ComNets)
# Kopernikusstr. 16, D-52074 Aachen, Germany
# phone: ++49-241-80-27910,
# fax: ++49-241-80-22242
# email: info@openwns.org
# www: http://www.openwns.org
# _____________________________________________________________________________
#
# openWNS is free software; you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License version 2 as published by the
# Free Software Foundation;
#
# openWNS is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################

import openwns

# TODO dbn: Cleanup and move to separate files

class UniformRandomDecoder(openwns.StaticFactoryClass):

    def __init__(self, parentLogger = None):
        openwns.StaticFactoryClass.__init__(self, "UniformRandomDecoder")
        self.initialPER = 0.1
        self.rolloffFactor = 1.0
        self.logger = openwns.logger.Logger("WNS", "UnifromRandomDecoder", True, parentLogger)

class ChaseCombiningDecoder(openwns.StaticFactoryClass):

    def __init__(self, parentLogger = None):
        openwns.StaticFactoryClass.__init__(self, "ChaseCombiningDecoder")
        self.logger = openwns.logger.Logger("WNS", "ChaseCombiningDecoder", True, parentLogger)


class HARQReceiver:

    def __init__(self, parentLogger=None):
        self.decoder = ChaseCombiningDecoder(parentLogger=parentLogger)

class HARQEntity:

    def __init__(self, parentLogger=None):
        self.receiverConfig = HARQReceiver(parentLogger=parentLogger)

class HARQ(openwns.StaticFactoryClass):

    def __init__(self, parentLogger=None):
        openwns.StaticFactoryClass.__init__(self, "harq")
        self.numSenderProcesses = 100
        self.numReceiverProcesses = 100
        self.numRVs = 1
        self.harqEntity = HARQEntity()
        self.logger = openwns.logger.Logger("WNS", "HARQ", True, parentLogger)
