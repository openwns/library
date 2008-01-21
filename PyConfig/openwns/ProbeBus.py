import openwns.Logger
import copy

class ProbeBusRegistry:
        def __init__(self, probeBusPrototype):
                self.prototype = probeBusPrototype

class ProbeBus:
        """ Base configuration class for all probe busses. It keeps track of
        the observing relationship between probe busses."""
        def __init__(self, outputTag):
                self.subject = None
                self.observers = []
		self.outputTag = outputTag

        def observe(self, otherProbeBus):
		assert self.subject == None, "I am already observing another subject: "+str(self.subject)
		self.subject = otherProbeBus
                otherProbeBus.__addObserver(self)

	def listen(self, otherProbeBus):
		""" listen behaves like observe but returns the otherProbeBus. You can use this for chaining ProbeBusses, e.g. if
                    you want to log measurements on an already defined ProbeBus you can simply wrap the instatiation of this ProbeBus
                    with a LoggingProbeBus like this:
		      # Without Logging
		      myProbeBus = MasterProbeBus()

		      # With Logging
		      myProbeBus = LoggingProbeBus().listen(MasterProbeBus())
	        """
		self.observe(otherProbeBus)
		return otherProbeBus

        def __addObserver(self, observer):
                self.observers.append(observer)

	def getOutputTags(self):
		tags = []
		if self.subject != None:
			tags += self.subject.getOutputTags()

		tags.append(self.outputTag)
		return tags

class SubTree:
	"""
	Class that represents and handles subtrees of a probebus hierarchy.
	"""
	probeBusID = None
	top = None

	def __init__(self, _probeBusID = ""):
		self.probeBusID = _probeBusID
		self.top = []

	def empty(self):
		return self.top == []

	def getBottom(self):
		def getLeafs(probeBus):
			if probeBus.observers == []:
				yield probeBus
			else:
				for bus in probeBus.observers:
					for item in getLeafs(bus):
						yield item
		bottom = []
		for upper in self.top:
			for bus in getLeafs(upper):
				bottom.append(bus)
		return bottom

	def chain(self, other):
		if self.empty():
			other.probeBusID = self.probeBusID
			return other
		if other.empty():
			return self
		assert isinstance(other, SubTree), "Can only chain other subtrees!"
		template = other
		newBottom = []
		for b in self.getBottom():
			newChild = copy.deepcopy(template)
			newChild.__observe(b)
			newBottom += newChild.getBottom()
		s = SubTree(self.probeBusID)
		s.top = self.top
		return s

	def dump(self):
		def recursiveWalk(probeBus, currentLevel=0):
			yield probeBus, currentLevel
			for bus in probeBus.observers:
				for item, level in recursiveWalk(bus, currentLevel+1):
					yield item, level

		print "SubTree: '"+self.probeBusID+"'"
		for upper in self.top:
			for bus, level in recursiveWalk(upper):
				indentation = ""
				for ii in xrange(level): indentation += "  "
				print "%s%d: %s %s" % (indentation, level, str(bus), str(bus.getOutputTags()))

	def __observe(self, parent):
		# plug all the top busses into the parent
		for bus in self.top:
			bus.observe(parent)


class SubTreeRegistry:
        def __init__(self):
                self.subtrees = []

        def insertSubTree(self, _subtree):
		assert isinstance(_subtree, SubTree)
                self.subtrees.append(_subtree)

        def insertProbeBus(self, _probeBusID, _probeBus):
		s = SubTree(_probeBusID)
		s.top.append(_probeBus)
		self.subtrees.append(s)

	def clear(self):
		self.subtrees = []

	def getSubTree(self, name):
		for subtree in self.subtrees:
			if subtree.probeBusID == name:
				return subtree
		raise Exception("'"+name+"' not found in "+str(self))

class MasterProbeBus(ProbeBus):
        """ The MasterProbeBus always accepts and always forwards. Probably used as
        prototype in the ProbeBusRegistry"""
        nameInFactory = "MasterProbeBus"

        def __init__(self):
                ProbeBus.__init__(self,"")

class SettlingTimeGuard(ProbeBus):
        """ The SettlingTimeGuard only accepts if the global settling time (transient phase)
        has elapsed"""
        nameInFactory = "SettlingTimeGuard"

        def __init__(self):
                ProbeBus.__init__(self,"")

class LoggingProbeBus(ProbeBus):
        """ The LoggingProbeBus always accepts and logs the message to the logging subsystem.
        """
        nameInFactory = "LoggingProbeBus"

        def __init__(self, parentLogger=None):
                ProbeBus.__init__(self,"")
                self.logger = openwns.Logger.Logger("WNS", "ProbeBus", True, parentLogger)

class PythonProbeBus(ProbeBus):
        """ Use the PythonProbeBus to do all your probing work in python. Specify what to do
        in accepts, onMeasurement, output from within your configuration file."""
        nameInFactory = "PythonProbeBus"

        def _dummyOnMeasurement(timestamp, value, reg):
                pass

        def _dummyOutput():
                pass

        def __init__(self, acceptsFunction, onMeasurementFunction = _dummyOnMeasurement, outputFunction = _dummyOutput):
                ProbeBus.__init__(self,"")
                self.accepts = acceptsFunction
                self.onMeasurement = onMeasurementFunction
                self.output = outputFunction
                self.reportErrors = True

class TimeWindowProbeBus(ProbeBus):
        """ Only accepts for a certain time window given by start and end time"""

        nameInFactory = "TimeWindowProbeBus"

        def __init__(self, start, end):
                ProbeBus.__init__(self,"")
                self.start = start
                self.end = end
