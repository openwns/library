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

# QoS classes
# corresponds to C++ file QoS/QoSClasses.hpp
from openwns.pyconfig import attrsetter

numberOfQoSClasses = 8
undefinedQosClass = "UNDEFINED"
conversationalQosClass = "CONVERSATIONAL"
streamingQosClass ="STREAMING"
interactiveQosClass = "INTERACTIVE"
backgroundQosClass = "BACKGROUND"
priorityBest  = 0
priorityWorst = 6

# one element of the classes:
class QoSClass:
    name = None
    number = None
    priority = None
    def __init__(self, name, number, **kw):
        self.name   = name
        self.number = number
        attrsetter(self, kw)

class QoSClasses:
    mapEntries = None

    def __init__(self, **kw):
        self.mapEntries = []
        # this list must be consistent with C++ code in service/qos/QoSClasses.hpp
        # uppercase is important!
        self.mapEntries.append(QoSClass("UNDEFINED",0,priority=6))
        self.mapEntries.append(QoSClass("PCCH",1,priority=0))
        self.mapEntries.append(QoSClass("PHICH",2,priority=1))
        self.mapEntries.append(QoSClass("DCCH",3,priority=2))
        self.mapEntries.append(QoSClass("CONVERSATIONAL",4,priority=3))
        self.mapEntries.append(QoSClass("STREAMING",5,priority=4))
        self.mapEntries.append(QoSClass("INTERACTIVE",6,priority=5))
        self.mapEntries.append(QoSClass("BACKGROUND",7,priority=6))
        attrsetter(self, kw)
    
    def addQoSClass(self, name, number):
        tmp = QoSClass(name, number)
        self.mapEntries.append(tmp)

    def getNumberOfQosClasses(self):
        return self.mapEntries.size()

    def getMaxPriority(self):
        maxprio = 0
        for mapEntry in self.mapEntries:
            if mapEntry.priority>maxprio:
                maxprio=mapEntry.priority
        return maxprio

    def getQosClassByName(self, name):
        qosClass = undefinedQosClass
        for mapEntry in self.mapEntries:
            if mapEntry.name == name:
                qosClass = mapEntry
        return qosClass

    def verifyQosName(self, name):
        qosClass = undefinedQosClass
        for mapEntry in self.mapEntries:
            if mapEntry.name == name:
                qosClass = mapEntry
        return qosClass.name

    def getPriorityByQosClassName(self, name):
        priority = priorityWorst
        for mapEntry in self.mapEntries:
            if mapEntry.name == name:
                priority = mapEntry.priority
        return priority
