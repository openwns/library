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

class MeasurementSource(object):

	def __init__(self):
		object.__init__(self)
		self.observers = []

	def addObserver(self, probeBus):
		self.observers.append(probeBus)
		return probeBus

class ProbeBus(MeasurementSource):

	def __init__(self):
		MeasurementSource.__init__(self)

	def observe(self, probeBus):
		probeBus.addObserver(self)
		return probeBus

class ProbeBusRegistry(object):

        def __init__(self):
                super(ProbeBusRegistry, self).__init__()
		self.measurementSources = {}
                self.logger = openwns.logger.Logger("WNS", "ProbeBusRegistry", True)

	def getMeasurementSource(self, probeBusID):
		if not self.measurementSources.has_key(probeBusID):
			self.measurementSources[probeBusID] = MeasurementSource()

		return self.measurementSources[probeBusID]

class PassThroughProbeBus(ProbeBus):
        """ The PassThroughProbeBus always accepts and always forwards. """
        nameInFactory = "PassThroughProbeBus"

        def __init__(self):
                ProbeBus.__init__(self)

class SettlingTimeGuard(ProbeBus):
        """ The SettlingTimeGuardProbeBus only accepts if the global settling time (transient phase)
        has elapsed"""
        nameInFactory = "SettlingTimeGuardProbeBus"

        def __init__(self):
                ProbeBus.__init__(self)

class LoggingProbeBus(ProbeBus):
        """ The LoggingProbeBus always accepts and logs the message to the logging subsystem.
        """
        nameInFactory = "LoggingProbeBus"

        def __init__(self, parentLogger=None):
                ProbeBus.__init__(self)
                self.logger = openwns.logger.Logger("WNS", "ProbeBus", True, parentLogger)

class PythonProbeBus(ProbeBus):
        """ Use the PythonProbeBus to do all your probing work in python. Specify what to do
        in accepts, onMeasurement, output from within your configuration file."""
        nameInFactory = "PythonProbeBus"

        def _dummyOnMeasurement(timestamp, value, reg):
                pass

        def _dummyOutput():
                pass

        def __init__(self, acceptsFunction, onMeasurementFunction = _dummyOnMeasurement, outputFunction = _dummyOutput):
                ProbeBus.__init__(self)
                self.accepts = acceptsFunction
                self.onMeasurement = onMeasurementFunction
                self.output = outputFunction
                self.reportErrors = True

class TimeWindowProbeBus(ProbeBus):
        """ Only accepts for a certain time window given by start and end time"""

        nameInFactory = "TimeWindowProbeBus"

        def __init__(self, start, end):
                ProbeBus.__init__(self)
                self.start = start
                self.end = end

class LogEvalProbeBus(ProbeBus):
        """ The LogEval ProbeBus always accepts and logs the values into a file.
        """
        nameInFactory = "LogEvalProbeBus"

	outputDir = None

	outputFilename = None

	format = None

	timePrecision = None

	valuePrecision = None

        def __init__(self, **kwargs):
		ProbeBus.__init__(self)
		outputDir = openwns.getSimulator().outputDir
		openwns.pyconfig.attrsetter(self, kwargs)

