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

import openwns.logger
import openwns
import copy
from openwns.pyconfig import attrsetter

# class SubTreeRegistry(object):

#         def __init__(self):
#                 super(SubTreeRegistry, self).__init__()
#                 self.subtrees = []

#         def insertSubTree(self, _subtree):
# 		assert isinstance(_subtree, SubTree)
#                 self.subtrees.append(_subtree)

#         def removeSubTree(self, _subtree):
# 		assert isinstance(_subtree, SubTree)
#                 self.subtrees.remove(_subtree)

#         def insertSubTrees(self, _subtrees):
# 		for tree in _subtrees:
# 			assert isinstance(tree, SubTree)
# 			self.subtrees.append(tree)

#         def insertProbeBus(self, _probeBusID, _probeBus):
# 		s = SubTree(_probeBusID)
# 		s.top.append(_probeBus)
# 		self.subtrees.append(s)

# 	def clear(self):
# 		self.subtrees = []

# 	def getSubTree(self, name):
# 		for subtree in self.subtrees:
# 			if subtree.probeBusID == name:
# 				return subtree
# 		raise Exception("'"+name+"' not found in "+str(self))


class MeasurementSource(object):

	def __init__(self):
		object.__init__(self)
		self.observers = []

	def addObserver(self, probeBus):
		self.observers.append(probeBus)
		return probeBus

class ProbeBusRegistry(object):

        def __init__(self):
                super(ProbeBusRegistry, self).__init__()
		self.measurementSources = {}

	def getMeasurementSource(self, probeBusID):
		if not self.measurementSources.has_key(probeBusID):
			self.measurementSources[probeBusID] = MeasurementSource()

		return self.measurementSources[probeBusID]

class ProbeBus(MeasurementSource):

	def __init__(self):
		MeasurementSource.__init__(self)

	def observe(self, probeBus):
		probeBus.addObserver(self)
		return probeBus

# class ProbeBus:
#         """ Base configuration class for all probe busses. It keeps track of
#         the observing relationship between probe busses."""
#         def __init__(self, outputTag):
#                 self.subject = None
#                 self.observers = []
# 		self.outputTag = outputTag

#         def observe(self, otherProbeBus):
# 		assert self.subject == None, "I am already observing another subject: "+str(self.subject)
# 		self.subject = otherProbeBus
#                 otherProbeBus.__addObserver(self)

# 	def listen(self, otherProbeBus):
# 		""" listen behaves like observe but returns the otherProbeBus. You can use this for chaining ProbeBusses, e.g. if
#                     you want to log measurements on an already defined ProbeBus you can simply wrap the instatiation of this ProbeBus
#                     with a LoggingProbeBus like this:
# 		      # Without Logging
# 		      myProbeBus = MasterProbeBus()

# 		      # With Logging
# 		      myProbeBus = LoggingProbeBus().listen(MasterProbeBus())
# 	        """
# 		self.observe(otherProbeBus)
# 		return otherProbeBus

#         def __addObserver(self, observer):
#                 self.observers.append(observer)

# 	def getOutputTags(self):
# 		tags = []
# 		if self.subject != None:
# 			tags += self.subject.getOutputTags()

# 		tags.append(self.outputTag)
# 		return tags

# class SubTree:
# 	"""
# 	Class that represents and handles subtrees of a probebus
# 	hierarchy. The leafs are instances of ProbeBusses. This is a
# 	management structure.
# 	"""
# 	probeBusID = None
# 	top = None

# 	def __init__(self, _probeBusID = ""):
# 		self.probeBusID = _probeBusID
# 		self.top = []

#         def addToTop(self, busses):
#                 """ takes a list or one instance of a probe bus """
#                 if not isinstance(busses, list):
#                         busses = [busses]
#                 for bus in busses:
#                         if not isinstance(bus, ProbeBus):
#                                 raise Exception("Only instances of ProbeBus allowed")
#                         self.top.append(bus)


# 	def empty(self):
# 		return self.top == []

# 	def getBottom(self):
# 		def getLeafs(probeBus):
# 			if probeBus.observers == []:
# 				yield probeBus
# 			else:
# 				for bus in probeBus.observers:
# 					for item in getLeafs(bus):
# 						yield item
# 		bottom = []
# 		for upper in self.top:
# 			for bus in getLeafs(upper):
# 				bottom.append(bus)
# 		return bottom

# 	def chain(self, other):
# 		""" Adds deep copies of other to the leafs of self"""

# 		if self.empty():
# 			other.probeBusID = self.probeBusID
# 			return other
# 		if other.empty():
# 			return self
# 		assert isinstance(other, SubTree), "Can only chain other subtrees!"
# 		newBottom = []
# 		for b in self.getBottom():
# 			newChild = copy.deepcopy(other)
# 			newChild.__observe(b)
# 			newBottom += newChild.getBottom()
# 		s = SubTree(self.probeBusID)
# 		s.top = self.top
# 		return s

# 	def dump(self):
# 		def recursiveWalk(probeBus, currentLevel=0):
# 			yield probeBus, currentLevel
# 			for bus in probeBus.observers:
# 				for item, level in recursiveWalk(bus, currentLevel+1):
# 					yield item, level

# 		print "SubTree: '"+self.probeBusID+"'"
# 		for upper in self.top:
# 			for bus, level in recursiveWalk(upper):
# 				indentation = ""
# 				for ii in xrange(level): indentation += "  "
# 				print "%s%d: %s %s" % (indentation, level, str(bus), str(bus.getOutputTags()))

# 	def __observe(self, parent):
# 		# plug all the top busses into the parent
# 		for bus in self.top:
# 			bus.observe(parent)

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
		attrsetter(self, kwargs)

