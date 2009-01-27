# QoS classes
# corresponds to C++ file QoS/QoSClasses.hpp
from wns.PyConfig import Sealed
from wns.PyConfig import attrsetter

numberOfQoSClasses = 5
undefinedQosClass = "UNDEFINED"
bestEffortQosClass = "BESTEFFORT"
priorityBest  = 0
priorityWorst = 4

# one element of the classes:
class QoSClass:
    name = None
    number = None
    priority = None
    def __init__(self, name, number, **kw):
        self.name   = name
        self.number = number
        #self.priority = priority # mapping
        attrsetter(self, kw)

class QoSClasses:
    mapEntries = None

    def __init__(self, **kw):
        self.mapEntries = []
        # this list must be consistent with C++ code in QoS/QoSClasses.hpp
        # uppercase is important!
        self.mapEntries.append(QoSClass("UNDEFINED",0,priority=4))
        self.mapEntries.append(QoSClass("CONTROLPLANE",1,priority=1))
        self.mapEntries.append(QoSClass("REALTIME",2,priority=2))
        self.mapEntries.append(QoSClass("GUARANTEEDTP",3,priority=3))
        self.mapEntries.append(QoSClass("BESTEFFORT",4,priority=4))
        attrsetter(self, kw)
    
    def addQoSClass(self, name, number):
        # items must be added in order of ascending sinr ranges
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

