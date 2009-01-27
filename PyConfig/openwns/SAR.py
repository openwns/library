###############################################################################
# This file is part of openWNS (open Wireless Network Simulator)
# _____________________________________________________________________________
#
# Copyright (C) 2004-2007
# Chair of Communication Networks (ComNets)
# Kopernikusstr. 5, D-52074 Aachen, Germany
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

from openwns.pyconfig import attrsetter
from openwns.logger import Logger
import openwns.FUN

class SAR(openwns.FUN.FunctionalUnit):
    __plugin__ = 'wns.sar.SAR'

    segmentSize = 42
    logger = None
    preserving = True
    headerSize = 1

    useSuspendProbe = False
    suspendProbeName = "timeBufferEmpty"

    def __init__(self, segmentSize, **kw):
        self.segmentSize = segmentSize
        attrsetter(self, kw)

class Fixed(SAR):
    __plugin__ = 'wns.sar.Fixed'
    name = "SAR Fixed"

    def __init__(self, segmentSize, parentLogger = None, **kw):
        super(Fixed, self).__init__(segmentSize, **kw)
        self.logger = Logger('WNS','FixedSAR',True, parentLogger)

class Soft(SAR):
    __plugin__ = 'wns.sar.Soft'
    name = "SAR Soft"
    PERProvider = "Name of the PER Provider Functional unit, e.g. the MACr used"
    useProbe  = False
    probeName = "PER"

    def __init__(self, segmentSize, PERProviderName, parentLogger = None, **kw):
        super(Soft, self).__init__(segmentSize, **kw)
        self.PERProvider = PERProviderName
        self.logger = Logger('WNS','SoftSAR',True, parentLogger)
