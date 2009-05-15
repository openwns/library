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
import openwns.logger

# The strategies for Dynamic Subcarrier Assignment (DSA)
class DSAStrategy(object):
    nameInDSAStrategyFactory = None
    oneUserOnOneSubChannel = None
    adjacentSubchannelsOnUplink = None
    logger = None
    def __init__(self, parentLogger = None, oneUserOnOneSubChannel = True, adjacentSubchannelsOnUplink = False, **kw):
        #self.logger = openwns.logger.Logger("WNS", "DSAStrategy", True, parentLogger)
        self.logger = openwns.logger.Logger("WNS", ".".join(["DSAStrategy",self.nameInDSAStrategyFactory]), True, parentLogger)
        self.oneUserOnOneSubChannel = oneUserOnOneSubChannel
        self.adjacentSubchannelsOnUplink = adjacentSubchannelsOnUplink
        attrsetter(self, kw)
    def setParentLogger(self,parentLogger = None):
        #self.logger = openwns.logger.Logger("WNS", "DSAStrategy", True, parentLogger)
        self.logger = openwns.logger.Logger("WNS", ".".join(["DSAStrategy",self.nameInDSAStrategyFactory]), True, parentLogger)
        self.logger.enabled = parentLogger.enabled

class DoNotUseDSA(DSAStrategy):
    requiresCQI = False
    def __init__(self, **kw):
        self.nameInDSAStrategyFactory = "DoNotUseDSA"
        super(DoNotUseDSA,self).__init__(**kw)

class LinearFFirst(DSAStrategy):
    requiresCQI = False
    useRandomChannelAtBeginning = None
    def __init__(self, useRandomChannel = False, **kw):
        self.nameInDSAStrategyFactory = "LinearFFirst"
        super(LinearFFirst,self).__init__(**kw)
        self.useRandomChannelAtBeginning = useRandomChannel

class BestChannel(DSAStrategy):
    requiresCQI = True
    useRandomChannelAtBeginning = None
    def __init__(self, useRandomChannel = False, **kw):
        self.nameInDSAStrategyFactory = "BestChannel"
        super(BestChannel,self).__init__(**kw)
        self.useRandomChannelAtBeginning = useRandomChannel


class BestCapacity(DSAStrategy):
    requiresCQI = True
    useRandomChannelAtBeginning = None
    def __init__(self, useRandomChannel = False, **kw):
        self.nameInDSAStrategyFactory = "BestCapacity"
        super(BestCapacity,self).__init__(**kw)
        self.useRandomChannelAtBeginning = useRandomChannel

