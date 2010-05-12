from openwns.evaluation import *

def installEvaluation(sim):

    sourceName = sim.memConsumptionProbeBusName
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.appendChildren(TimeSeries())
    node.appendChildren(Moments())

    sourceName = sim.simTimeProbeBusName
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.appendChildren(TimeSeries())
    node.appendChildren(Moments())

    sourceName = sim.cpuCyclesProbeBusName
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.appendChildren(Moments())
