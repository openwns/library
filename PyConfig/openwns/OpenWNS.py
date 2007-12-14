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

import openwns.EventScheduler
import openwns.Logger
import openwns.RNG

class OpenWNS(object):
    """ Master configuration

    This class keeps the configuration at the top level. The
    simulator expects an instance of this Type name 'WNS' in the
    config file.
    """

    __slots__ = ["eventScheduler", "masterLogger", "rng", "__postProcessingFuncs", "logger", "maxSimTime", "eventSchedulerMonitor"]

    def __init__(self):
        # set defaults
        self.eventScheduler = openwns.EventScheduler.Map()
        self.masterLogger = openwns.Logger.MasterLogger()
        self.rng = openwns.RNG.RNG(useRandomSeed = False)
        self.logger = openwns.Logger.Logger("WNS", "Application", True)
        self.maxSimTime = 0.0
        self.eventSchedulerMonitor = openwns.EventScheduler.Monitor()

        # private: keeps a list of tasks for post processing
        self.__postProcessingFuncs = []

    def addPostProcessing(self, func):
        """ Add tasks to the post processing hook

        - func must be callable
        - func must return bool (true on successful operation)
        """
        self.__postProcessingFuncs.append(func)

    def postProcessing(self):
        """ This will be called from the simulator

        Tasks registered for post processing will be executed until a
        task returns False or no tasks are left
        """
        for func in self.__postProcessingFuncs:
            if not func(self):
                return False
        return True

