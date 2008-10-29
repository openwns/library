from openwns.evaluation import *

def installEvaluation(sim):

    sourceName = sim.memConsumptionProbeBusName
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.appendChildren(TimeSeries())
    node.appendChildren(Moments())
