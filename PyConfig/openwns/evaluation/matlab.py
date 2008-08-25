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
import openwns.logger
from openwns.pyconfig import attrsetter
import openwns.interface

class matlabPrintable:
    """ Derive from this class if you want to output all class members
    in a Matlab readable format."""
    def printString(self):
        raise NotImplementedError
    def printMatlab(self):
        #print dir(self) # overview
        resultString=''
        for item in dir(self):
            if (item.find('__')>=0): # internal
                continue # omit
            value = eval("self."+item)
            if ("%s"%value == 'True'):
                itemString = "%s"%item + " = true;\n"
            elif ("%s"%value == 'False'):
                itemString = "%s"%item + " = false;\n"
            else:
                try: # numeric?
                    itemString = "%s"%item + " = %f;"%value + "\n"
                except: # not numeric
                    itemString = "%s"%item + " = \'%s\';"%value + "\n"
                    if (itemString.find('<')>=0): # object
                        itemString='' # omit
                    if (itemString.find('{')>=0): # list
                        itemString='' # omit
            resultString = resultString + itemString
        return resultString
