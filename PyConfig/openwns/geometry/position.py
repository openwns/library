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

import math

class Position:
    """ This represents a geometric position in space (3D). Unit is meters [m] """
    __slots__ = ["x", "y", "z"]

    def __init__(self, x=0, y=0,z=0):
        self.x = x
        self.y = y
        self.z = z

    def toString(self):
        return ('%d %d %d' % (self.x, self.y, self.z))

class Vector:
    """ This represents a geometric shift in space (3D). Unit is meters [m] """
    __slots__ = ["x", "y", "z"]

    def __init__(self, x=0, y=0,z=0):
        self.x = x
        self.y = y
        self.z = z

    def toString(self):
        return ('%d %d %d' % (self.x, self.y, self.z))

class BoundingBox:
    """ Box with x,y,z dimension """
    __slots__ = ["xmin","xmax", "ymin","ymax", "zmin","zmax"]

    def __init__(self):
        self.xmin = 1e9
        self.ymin = 1e9
        self.zmin = 1e9
        self.xmax = -1e9
        self.ymax = -1e9
        self.zmax = -1e9

    def extendToIncludePosition(self,point):
        if (point.x<self.xmin):
            self.xmin=point.x
        if (point.y<self.ymin):
            self.ymin=point.y
        if (point.z<self.zmin):
            self.zmin=point.z
        if (point.x>self.xmax):
            self.xmax=point.x
        if (point.y>self.ymax):
            self.ymax=point.y
        if (point.z>self.zmax):
            self.zmax=point.z

    def roundToNextNiceNumber(self):
        self.xmin=math.floor(self.xmin)
        self.ymin=math.floor(self.ymin)
        self.zmin=math.floor(self.zmin)
        self.xmax=math.ceil(self.xmax)
        self.ymax=math.ceil(self.ymax)
        self.zmax=math.ceil(self.zmax)
