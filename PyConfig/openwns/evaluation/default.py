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

def installPersVoIPSchedulerEvaluation(sim, settlingTime, loggingStations, stationCount):

    sourceName = 'scheduler.persistentvoip.FrameOccupationFairness'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Frame Occupation Fairness',
                                     minXValue = 0,
                                     maxXValue = 1,
                                     resolution = 100))

    sourceName = 'scheduler.persistentvoip.ActiveConnections'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Active Connections',
                                     minXValue = 0,
                                     maxXValue = stationCount,
                                     resolution = stationCount))

    sourceName = 'scheduler.persistentvoip.QueuedConnections'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Queued Connections',
                                     minXValue = 0,
                                     maxXValue = stationCount,
                                     resolution = stationCount))

    sourceName = 'scheduler.persistentvoip.AllActiveConnections'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'All Active Connections',
                                     minXValue = 0,
                                     maxXValue = stationCount,
                                     resolution = stationCount))

    sourceName = 'scheduler.persistentvoip.TimeRelocatedConnections'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Time Relocated Connections',
                                     minXValue = 0,
                                     maxXValue = stationCount,
                                     resolution = stationCount))

    sourceName = 'scheduler.persistentvoip.FreqRelocatedConnections'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Frequency Relocated Connections',
                                     minXValue = 0,
                                     maxXValue = stationCount,
                                     resolution = stationCount))

    sourceName = 'scheduler.persistentvoip.NumberOfPDCCHs'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Number of PDCCHs',
                                     minXValue = 0,
                                     maxXValue = stationCount,
                                     resolution = stationCount))

    sourceName = 'scheduler.persistentvoip.NumberOfPersRelocationPDCCHs'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Number of Persistent Relocation PDCCHs',
                                     minXValue = 0,
                                     maxXValue = stationCount,
                                     resolution = stationCount))

    sourceName = 'scheduler.persistentvoip.NumberOfPersSetupPDCCHs'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Number of Persistent Setup PDCCHs',
                                     minXValue = 0,
                                     maxXValue = stationCount,
                                     resolution = stationCount))    

    sourceName = 'scheduler.persistentvoip.NumberOfDynamicPDCCHs'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Number of Dynamic PDCCHs',
                                     minXValue = 0,
                                     maxXValue = stationCount,
                                     resolution = stationCount))

    sourceName = 'scheduler.persistentvoip.NumberOfSID_PDCCHs'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Number of SID PDCCHs',
                                     minXValue = 0,
                                     maxXValue = stationCount,
                                     resolution = stationCount))

    sourceName = 'scheduler.persistentvoip.NumberOfOtherFramePDCCHs'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Number of other Frame PDCCHs',
                                     minXValue = 0,
                                     maxXValue = stationCount,
                                     resolution = stationCount))

    sourceName = 'scheduler.persistentvoip.NumberOfHARQ_PDCCHs'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Number of HARQ PDCCHs',
                                     minXValue = 0,
                                     maxXValue = stationCount,
                                     resolution = stationCount))

    sourceName = 'scheduler.persistentvoip.NumberOfHARQ_Users'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Number of HARQ Users',
                                     minXValue = 0,
                                     maxXValue = stationCount,
                                     resolution = stationCount))

