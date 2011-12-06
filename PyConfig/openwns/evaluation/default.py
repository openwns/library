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


    sourceName = 'scheduler.persistentvoip.failedReactivations'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Failed Reactivations',
                                     minXValue = 0,
                                     maxXValue = 1,
                                     resolution = 100))
    node.getLeafs().appendChildren(Plot2D(xDataKey = 'schedUserID',
                                            minX = 0,
                                            maxX = max(loggingStations) + 1,
                                            resolution = max(loggingStations) + 1,
                                            statEvals = ['mean','trials']))

    sourceName = 'scheduler.persistentvoip.failedSetup'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Failed Setups',
                                     minXValue = 0,
                                     maxXValue = 1,
                                     resolution = 100))
    node.getLeafs().appendChildren(Plot2D(xDataKey = 'schedUserID',
                                            minX = 0,
                                            maxX = max(loggingStations) + 1,
                                            resolution = max(loggingStations) + 1,
                                            statEvals = ['mean','trials']))

    sourceName = 'scheduler.persistentvoip.failedFreqRelocation'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Failed Freq Relocations',
                                     minXValue = 0,
                                     maxXValue = 1,
                                     resolution = 100))
    node.getLeafs().appendChildren(Plot2D(xDataKey = 'schedUserID',
                                            minX = 0,
                                            maxX = max(loggingStations) + 1,
                                            resolution = max(loggingStations) + 1,
                                            statEvals = ['mean','trials']))

    sourceName = 'scheduler.persistentvoip.failedTimeRelocation'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Failed Time Relocations',
                                     minXValue = 0,
                                     maxXValue = 1,
                                     resolution = 100))
    node.getLeafs().appendChildren(Plot2D(xDataKey = 'schedUserID',
                                            minX = 0,
                                            maxX = max(loggingStations) + 1,
                                            resolution = max(loggingStations) + 1,
                                            statEvals = ['mean','trials']))

    sourceName = 'scheduler.persistentvoip.failedTimeFreqRelocation'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Need more from other frames',
                                     minXValue = 0,
                                     maxXValue = 1,
                                     resolution = 100))
    node.getLeafs().appendChildren(Plot2D(xDataKey = 'schedUserID',
                                            minX = 0,
                                            maxX = max(loggingStations) + 1,
                                            resolution = max(loggingStations) + 1,
                                            statEvals = ['mean','trials']))

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

    sourceName = 'scheduler.persistentvoip.TimeFreqRelocatedConnections'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Time & Frequency Relocated Connections',
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

    sourceName = 'scheduler.persistentvoip.NumberOfPersSetupTimeRelocatedPDCCHs'
    node = openwns.evaluation.createSourceNode(sim, sourceName)
    node.getLeafs().appendChildren(SettlingTimeGuard(settlingTime = settlingTime))
    node.getLeafs().appendChildren(Accept(by='nodeID', ifIn = loggingStations, suffix='CenterCell'))
    node.getLeafs().appendChildren(PDF(name = sourceName,
                                     description = 'Number of Persistent Setup PDCCHs from earlier frames',
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


