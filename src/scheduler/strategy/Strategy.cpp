/*******************************************************************************
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2004-2009
 * Chair of Communication Networks (ComNets)
 * Kopernikusstr. 5, D-52074 Aachen, Germany
 * phone: ++49-241-80-27910,
 * fax: ++49-241-80-22242
 * email: info@openwns.org
 * www: http://www.openwns.org
 * _____________________________________________________________________________
 *
 * openWNS is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 2 as published by the
 * Free Software Foundation;
 *
 * openWNS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#include <WNS/scheduler/CallBackInterface.hpp>
#include <WNS/scheduler/RegistryProxyInterface.hpp>
#include <WNS/scheduler/strategy/Strategy.hpp>
#include <WNS/scheduler/strategy/StrategyInterface.hpp>
//#include <WNS/scheduler/SchedulingMap.hpp>
#include <WNS/PowerRatio.hpp>

using namespace wns::scheduler;
using namespace wns::scheduler::strategy;

Strategy::Strategy(const wns::pyconfig::View& config)
    : colleagues(),
      friends(),
      pyConfig(config),
      logger(config.get("logger"))
{
    /** @brief PyConfig Attribute: */
    simTimeType symbolDuration = config.get<double>("symbolDuration");
    /** @brief isTx=true for RS-TX (DL,UL) in (BS,UT,RN) */
    bool txMode = config.get<bool>("txMode"); // Python parameter
    /** @brief PyConfig Attribute:
        flag to determine whether doAdaptiveResourceScheduling cares for too low SINR */
    bool excludeTooLowSINR = config.get<bool>("excludeTooLowSINR");
    /** @brief PyConfig Attribute:
        flag to determine whether the strategy is a PowerControl Slave */
    bool powerControlSlave = config.get<bool>("powerControlSlave");
    /** @brief there are three positions for the scheduler... */
    PowerControlType powerControlType;

    MESSAGE_SINGLE(NORMAL, logger,"Strategy="<<config.get<std::string>("nameInStrategyFactory"));

    // there are three positions for the scheduler...
    if (txMode && !powerControlSlave) {
        // 1.) BS.Tx (DL) -> power from Python (myself)
        powerControlType = PowerControlDLMaster;
    } else if (!txMode && !powerControlSlave) {
        // 2.) BS.Rx (UL) -> power from Python (user)
        powerControlType = PowerControlULMaster;
    } else if (txMode && powerControlSlave) {
        // 3.) UT.Tx (UL) -> power from masterBurst
        powerControlType = PowerControlULSlave;
    } else {
        assure(0,"invalid case for powerControlType: txMode="<<txMode<<", powerControlSlave="<<powerControlSlave);
        // this luckily also happens when compiled in opt mode:
        throw wns::Exception("invalid case for powerControlType");
    }
    getNewSchedulerState();
    schedulerState->symbolDuration = symbolDuration;
    schedulerState->isTx = txMode;
    schedulerState->powerControlType = powerControlType;
    schedulerState->excludeTooLowSINR = excludeTooLowSINR; // default true
}

Strategy::~Strategy()
{
    if (colleagues.apcstrategy) delete colleagues.apcstrategy;
    if (colleagues.dsastrategy) delete colleagues.dsastrategy;
    if (colleagues.dsafbstrategy) delete colleagues.dsafbstrategy;
    // delete SchedulerState:
    if (schedulerState) {
        if (schedulerState->currentState) {
            schedulerState->clearMap();
            if (schedulerState->currentState->strategyInput) {
                schedulerState->currentState->strategyInput = NULL;
            }
            if (schedulerState->currentState->channelQualitiesOfAllUsers) {
                schedulerState->currentState->channelQualitiesOfAllUsers = ChannelQualitiesOfAllUsersPtr();
            }
            if (schedulerState->currentState->schedulingMap) {
                schedulerState->currentState->schedulingMap = SchedulingMapPtr(); // empty
            }
            if (schedulerState->currentState->bursts) {
                schedulerState->currentState->bursts = MapInfoCollectionPtr(); // empty
            }
            schedulerState->currentState->strategyInput = NULL;
            schedulerState->currentState = RevolvingStatePtr(); // empty
        }
        schedulerState = SchedulerStatePtr(); // empty
    }
}

void
Strategy::setColleagues(queue::QueueInterface* _queue,
                        grouper::GroupingProviderInterface* _grouper,
                        RegistryProxyInterface* _registry,
                        wns::scheduler::harq::HARQInterface* _harq
    )
{
    MESSAGE_SINGLE(NORMAL, logger,"Strategy::setColleagues(): creating DSA/APC strategies...");
    colleagues.queue = _queue;
    colleagues.grouper = _grouper;
    colleagues.registry = _registry;
    colleagues.harq = _harq; // may be NULL if no HARQ is there.

    assure(dynamic_cast<queue::QueueInterface*>(colleagues.queue), "Need access to the queue");
    assure(dynamic_cast<grouper::GroupingProviderInterface*>(colleagues.grouper), "Need access to the grouper");
    assure(dynamic_cast<RegistryProxyInterface*>(colleagues.registry), "Need access to the registry");
    std::string apcstrategyName   = pyConfig.get<std::string>("apcstrategy.nameInAPCStrategyFactory");
    std::string dsastrategyName   = pyConfig.get<std::string>("dsastrategy.nameInDSAStrategyFactory");
    std::string dsafbstrategyName = pyConfig.get<std::string>("dsafbstrategy.nameInDSAStrategyFactory");
    MESSAGE_SINGLE(NORMAL, logger,"dsastrategy   = "<<dsastrategyName);
    MESSAGE_SINGLE(NORMAL, logger,"dsafbstrategy = "<<dsafbstrategyName);
    MESSAGE_SINGLE(NORMAL, logger,"apcstrategy   = "<<apcstrategyName);

    // create the scheduling dsastrategy
    wns::scheduler::strategy::dsastrategy::DSAStrategyCreator* dsastrategyCreator = wns::scheduler::strategy::dsastrategy::DSAStrategyFactory::creator(dsastrategyName);
    colleagues.dsastrategy = dsastrategyCreator->create(pyConfig.get<wns::pyconfig::View>("dsastrategy"));
    assure(colleagues.dsastrategy, "DSAStrategy module creation failed");

    // create the scheduling dsafallbackstrategy
    wns::scheduler::strategy::dsastrategy::DSAStrategyCreator* dsafbstrategyCreator = wns::scheduler::strategy::dsastrategy::DSAStrategyFactory::creator(dsafbstrategyName);
    colleagues.dsafbstrategy = dsafbstrategyCreator->create(pyConfig.get<wns::pyconfig::View>("dsafbstrategy"));
    assure(colleagues.dsafbstrategy, "DSAfbStrategy module creation failed");

    // create the scheduling apcstrategy
    wns::scheduler::strategy::apcstrategy::APCStrategyCreator* apcstrategyCreator = wns::scheduler::strategy::apcstrategy::APCStrategyFactory::creator(apcstrategyName);
    colleagues.apcstrategy = apcstrategyCreator->create(pyConfig.get<wns::pyconfig::View>("apcstrategy"));
    assure(colleagues.apcstrategy, "APCStrategy module creation failed");

    assure(colleagues.dsastrategy  !=NULL,"dsastrategy=NULL");
    assure(colleagues.dsafbstrategy!=NULL,"dsafbstrategy=NULL");
    assure(colleagues.apcstrategy  !=NULL,"apcstrategy=NULL");

    colleagues.dsastrategy  ->setColleagues(colleagues.registry);
    colleagues.dsafbstrategy->setColleagues(colleagues.registry);
    colleagues.apcstrategy  ->setColleagues(colleagues.registry);

    assure(schedulerState!=SchedulerStatePtr(),"schedulerState must be valid");
    bool useCQI = colleagues.registry->getCQIAvailable();
    schedulerState->useCQI = useCQI;
    assure(!colleagues.dsafbstrategy->requiresCQI(),"dsafbstrategy must never require CQI");
    assure(useCQI || !colleagues.dsastrategy->requiresCQI(),"dsastrategy requires CQI");
    assure(useCQI || !colleagues.apcstrategy->requiresCQI(),"apcstrategy requires CQI");
    this->onColleaguesKnown(); // calls method of derived class for initialization
} // setColleagues

void
Strategy::onColleaguesKnown()
{
    MESSAGE_SINGLE(NORMAL, logger,"Strategy::onColleaguesKnown()");
}

void
Strategy::setFriends(wns::service::phy::ofdma::BFInterface* _ofdmaProvider)
{
    friends.ofdmaProvider = _ofdmaProvider;
    assure(friends.ofdmaProvider!=NULL,"ofdmaProvider==NULL => eirpLimited undefined");
    assure(schedulerState!=SchedulerStatePtr(),"schedulerState must be valid");
    /** @brief PyConfig Attribute:
        flag to determine whether the strategy is a PowerControl Slave */
    bool eirpLimited = friends.ofdmaProvider->isEIRPLimited();
    schedulerState->eirpLimited = eirpLimited;
    assure(colleagues.registry!=NULL,"colleagues.registry==NULL");
    bool isDL = colleagues.registry->getDL(); // <- this is set very late in Registry
    if ((isDL==true) && (schedulerState->isTx==false)) {
        MESSAGE_SINGLE(NORMAL, logger,"Strategy::setFriends(): misconfiguration: isDL="<<isDL<<", isTx="<<schedulerState->isTx);
        isDL=false; // workaround for WiMAC where registry->getDL() is not implemented
    }
    schedulerState->isDL = isDL;
    assure((isDL && (schedulerState->powerControlType == PowerControlDLMaster)) || !isDL,
           logger.getLoggerName()
           <<": invalid: powerControlType="<<schedulerState->powerControlType<<",isDL="<<isDL<<",isTx="<<schedulerState->isTx);
    // there are three positions for the scheduler...
    wns::scheduler::SchedulerSpotType schedulerSpot;
    if ( isDL && schedulerState->isTx ) {
        schedulerSpot = wns::scheduler::SchedulerSpot::DLMaster();
        assure(canHandleDL(), "DLMaster");
    } else if ( !isDL && !schedulerState->isTx ) {
        schedulerSpot = wns::scheduler::SchedulerSpot::ULMaster();
        assure(canHandleUL(), "ULMaster");
    } else if ( !isDL && schedulerState->isTx ) {
        schedulerSpot = wns::scheduler::SchedulerSpot::ULSlave();
        assure(canBeSlave(), "ULSlave scheduler needs SlaveAspect of strategy");
        assure(!schedulerState->useCQI,"cannot useCQI in slave scheduler");
    } else {
        assure(0,"invalid case for schedulerSpot: isDL="<<isDL<<", isTx="<<schedulerState->isTx);
        // this luckily also happens when compiled in opt mode:
        throw wns::Exception("invalid case for schedulerSpot");
    }
    schedulerState->schedulerSpot = schedulerSpot;
    //UserID myUserID = colleagues.registry->getMyUserID();
    //schedulerState->myUserID = myUserID;
    MESSAGE_SINGLE(NORMAL, logger,"Strategy::setFriends(): isDL="<<isDL<<", isTx="<<schedulerState->isTx
                   <<", schedulerSpot="<<wns::scheduler::SchedulerSpot::toString(schedulerSpot)
                   <<", useCQI="<<schedulerState->useCQI);
} // setFriends

SchedulerStatePtr
Strategy::getNewSchedulerState()
{
    MESSAGE_SINGLE(NORMAL, logger,"Strategy::getNewSchedulerState()");
    assure(schedulerState==SchedulerStatePtr(),"schedulerState must be empty before making a new one");
    schedulerState = SchedulerStatePtr(new SchedulerState(this));
    assure(schedulerState!=SchedulerStatePtr(),"schedulerState must be valid");
    return schedulerState;
}

SchedulerStatePtr
Strategy::revolveSchedulerState(const StrategyInput& strategyInput)
{
    assure(schedulerState!=SchedulerStatePtr(),"schedulerState must be valid");
    assure(colleagues.registry!=NULL,"registry==NULL");
    assure(&strategyInput!=NULL,"strategyInput==NULL");
    schedulerState->currentState = RevolvingStatePtr(new RevolvingState(&strategyInput));
    return schedulerState;
}

SchedulerStatePtr
Strategy::getSchedulerState()
{
    assure(schedulerState!=SchedulerStatePtr(),"schedulerState must be valid");
    return schedulerState;
}

MapInfoCollectionPtr
Strategy::getMapInfo() const {
    // if startScheduling has never been called but mapHandler calls getMapInfo(): return empty
    if (schedulerState->currentState==wns::scheduler::strategy::RevolvingStatePtr())
        return MapInfoCollectionPtr(new MapInfoCollection);
    MapInfoCollectionPtr bursts = schedulerState->currentState->bursts; // alias
    assure(bursts!=MapInfoCollectionPtr(),"bursts must not be NULL here");
    return bursts;
}

wns::scheduler::PowerCapabilities
Strategy::getPowerCapabilities(const UserID user) const
{
    /* This method changes the state "schedulerState->powerCapabilities".
       This is good and ok if all users are the same (default of all systems),
       but if they are different (future), APC must call this function anytime.
    */
    switch (schedulerState->powerControlType)
    {
    case PowerControlDLMaster:
    {
        schedulerState->powerCapabilities = colleagues.registry->getPowerCapabilities(); // my own power
        break;
    }
    case PowerControlULMaster:
    {
        if (user==NULL) { // peer unknown. Assume peer=UT.
            MESSAGE_SINGLE(NORMAL, logger, "getPowerCapabilities(NULL): asking registry...");
            schedulerState->powerCapabilities = colleagues.registry->getPowerCapabilities(user); // from peer
            MESSAGE_SINGLE(NORMAL, logger, "getPowerCapabilities(NULL): nominal="<<schedulerState->powerCapabilities.nominalPerSubband);
            return schedulerState->powerCapabilities;
        }
        schedulerState->powerCapabilities = colleagues.registry->getPowerCapabilities(user); // from peer
        break;
    }
    case PowerControlULSlave:
    {
        if (schedulerState->defaultTxPower!=wns::Power()) {
            // don't use powerCapabilities but masterBurst instead
            assure(schedulerState->defaultTxPower!=wns::Power(),"undefined defaultTxPower");
            schedulerState->powerCapabilities.nominalPerSubband = schedulerState->defaultTxPower;
            schedulerState->powerCapabilities.maxPerSubband = schedulerState->defaultTxPower;
            schedulerState->powerCapabilities.maxOverall = schedulerState->defaultTxPower * 1000.0; // limit not used for slave
            //schedulerState->powerCapabilities.maxOverall = schedulerState->defaultTxPower * #subCh (N/A here);
        } else { // not given (by masterBurst) because there is an InputSchedulingMap (new method)
            assure(schedulerState->currentState->strategyInput->inputSchedulingMap != wns::scheduler::SchedulingMapPtr(), "need inputSchedulingMap with txPower information");
            // the power settings in inputSchedulingMap can be overwritten
            // if APC strategy is changed to use nominal power. Therefore we need these values:
            schedulerState->powerCapabilities = colleagues.registry->getPowerCapabilities(); // my own power
        }
        break;
    }
    default:
    {
        throw wns::Exception("invalid powerControlType");
    }
    } // switch(powerControlType)
    if (user!=NULL) {
        MESSAGE_SINGLE(NORMAL, logger, "getPowerCapabilities("<<user->getName()<<"): nominal="<<schedulerState->powerCapabilities.nominalPerSubband);
    } else {
        MESSAGE_SINGLE(NORMAL, logger, "getPowerCapabilities(NULL): nominal="<<schedulerState->powerCapabilities.nominalPerSubband);
    }
    assure(schedulerState->powerCapabilities.nominalPerSubband!=wns::Power(),"undefined power nominalPerSubband="<<schedulerState->powerCapabilities.nominalPerSubband);
    return schedulerState->powerCapabilities;
}

// perform master scheduling
StrategyResult // copy result and arguments (very lightweight datastructures)
Strategy::startScheduling(const StrategyInput& strategyInput)
{
    if (strategyInput.fChannels  < 1)          throw wns::Exception("Need at least one subBand");
    if (strategyInput.slotLength < 0.0)        throw wns::Exception("Invalid slotLength");
    if (strategyInput.slotLength < schedulerState->symbolDuration)    throw wns::Exception("Can't schedule on slot shorter than OFDM symbol");
    if (strategyInput.numberOfTimeSlots < 1)   throw wns::Exception("Need at least one TimeSlot");
    if (strategyInput.maxBeams   < 1)          throw wns::Exception("Need at least one beam");
    // ^ strategyInput.maxBeams>1 means beamforming or MIMO
    assure(isNewStrategy()
           || strategyInput.beamforming
           || strategyInput.maxBeams==1,
           "beamforming="<<strategyInput.beamforming<<" && maxBeams="<<strategyInput.maxBeams<<": MIMO not supported in old strategies");
    assure(!isNewStrategy()
           || !strategyInput.beamforming
           //|| strategyInput.maxBeams==1
           ,
           "beamforming="<<strategyInput.beamforming<<" && maxBeams="<<strategyInput.maxBeams<<": beamforming not (yet) working in new strategies");
    // prepare a new state for this timeFrame:
    schedulerState = revolveSchedulerState(strategyInput);
    if (strategyInput.inputSchedulingMap == wns::scheduler::SchedulingMapPtr()) // empty
    { // master scheduling on freshly created empty resources
        MESSAGE_SINGLE(NORMAL, logger, "inputSchedulingMap is empty");
        // the fresh Scheduling Map (empty and free) is prepared in the RevolvingStatePtr() constructor
    } else { // slave scheduling or preallocated SchedulingMap (default for new WinProSt)
        MESSAGE_SINGLE(NORMAL, logger, "inputSchedulingMap exists: "<<strategyInput.inputSchedulingMap->toString());
        assure(schedulerState->currentState->schedulingMap != wns::scheduler::SchedulingMapPtr(),"schedulingMap initialization failed");
        assure(schedulerState->currentState->schedulingMap == strategyInput.inputSchedulingMap,"schedulingMap must be set in revolveSchedulerState()");
    }
    wns::scheduler::SchedulingMapPtr schedulingMap = schedulerState->currentState->schedulingMap; // alias
    if (schedulerState->schedulerSpot != wns::scheduler::SchedulerSpot::ULSlave())
    { // master scheduling
        assure(strategyInput.mapInfoEntryFromMaster == MapInfoEntryPtr(),"mapInfoEntryFromMaster must be NULL");
        MESSAGE_BEGIN(NORMAL, logger, m, "startScheduling(master): ");
        if (strategyInput.frameNrIsValid())
            m << "frameNr=" << strategyInput.frameNr << ", ";
        m << "fChannels="   << strategyInput.fChannels
          << ", maxBeams="   << strategyInput.maxBeams
          << ", slotLength=" << strategyInput.slotLength;
        MESSAGE_END();
        // not necessary but these defaults may be optionally provided by the caller:
        schedulerState->setDefaultPhyMode(strategyInput.defaultPhyModePtr); // may be undefined (NULL) in most cases
        schedulerState->setDefaultTxPower(strategyInput.defaultTxPower); // may be undefined (0.0) in most cases
    } else { // slave scheduling
        // two ways of master input:
        // 1. old way: mapInfoEntryFromMaster
        // 2. new way: inputSchedulingMap
        if (strategyInput.mapInfoEntryFromMaster != MapInfoEntryPtr()) {
            assure(strategyInput.mapInfoEntryFromMaster != MapInfoEntryPtr(),"mapInfoEntryFromMaster must be non-NULL");
            MESSAGE_SINGLE(NORMAL, logger,"startScheduling(slave): fChannels="<<strategyInput.fChannels<<", subBand="<<strategyInput.mapInfoEntryFromMaster->subBand);
            MESSAGE_SINGLE(NORMAL, logger,"PhyMode="<<*(strategyInput.mapInfoEntryFromMaster->phyModePtr)<<", txPower="<<strategyInput.mapInfoEntryFromMaster->txPower);
            schedulerState->setDefaultPhyMode(strategyInput.mapInfoEntryFromMaster->phyModePtr);
            schedulerState->setDefaultTxPower(strategyInput.mapInfoEntryFromMaster->txPower);
        } else {
            assure(strategyInput.inputSchedulingMap != wns::scheduler::SchedulingMapPtr(), "slave scheduling requires inputSchedulingMap");
            MESSAGE_SINGLE(NORMAL, logger, "SlaveScheduling with given inputSchedulingMap...");
            // all PhyModes and TxPower are written in the inputSchedulingMap
            // I may only use those resources (subchannels,slots) where my userID is written into (UTx)
            // Before going to UT's PhyUser, take care to use the right userID!
            // it must be reverted (to BSx) in the PhyCommand to send to the right peer.
            assure(schedulerState->currentState->schedulingMap == strategyInput.inputSchedulingMap,"schedulingMap must be set in revolveSchedulerState()");
            // set empty all resources but keep userID, PhyMode, TxPower
            MESSAGE_SINGLE(NORMAL, logger, "inputSchedulingMap->processMasterMap()");
            schedulerState->currentState->schedulingMap->processMasterMap();
            MESSAGE_SINGLE(NORMAL, logger, "prepared master schedulingMap="<<schedulerState->currentState->schedulingMap->toString());
        }
    }
    schedulerState->clearMap(); // empty bursts result datastructure MapInfoCollection (not schedulingMap)
    // prepare CQI
    if (schedulerState->useCQI)
    {
        schedulerState->currentState->channelQualitiesOfAllUsers =
            ChannelQualitiesOfAllUsersPtr(new ChannelQualitiesOfAllUsers());
    }

    // Initialize the "working" datastructures
    // new burst result structure (part of the state):
    assure(schedulerState->currentState->bursts==MapInfoCollectionPtr(),"bursts must be NULL here");
    MapInfoCollectionPtr bursts = MapInfoCollectionPtr(new wns::scheduler::MapInfoCollection);
    schedulerState->currentState->bursts = bursts;
    // initialize helper strategies for DSA,APC:
    if (colleagues.dsastrategy!=NULL)
        colleagues.dsastrategy->initialize(schedulerState,schedulingMap);
    if (colleagues.dsafbstrategy!=NULL)
        colleagues.dsafbstrategy->initialize(schedulerState,schedulingMap);
    if (colleagues.apcstrategy!=NULL)
        colleagues.apcstrategy->initialize(schedulerState,schedulingMap);
    // this calls the derived strategies (various algorithms):
    StrategyResult strategyResult =
        this->doStartScheduling(schedulerState,schedulingMap);
    assure(strategyResult.schedulingMap==schedulingMap,"schedulingMap mismatch");
    if (colleagues.apcstrategy!=NULL)
        colleagues.apcstrategy->postProcess(schedulerState,schedulingMap);
    //schedulingMapReady(strategyResult); // make the callBacks

    return strategyResult;
} // startScheduling

/** @brief default implementation to support the old scheduler strategies.
    Please overload in all new strategies. Do not use this for new designs. */
StrategyResult
Strategy::doStartScheduling(SchedulerStatePtr schedulerState,
                            SchedulingMapPtr schedulingMap)
{
    assure(schedulerState->currentState!=RevolvingStatePtr(),"currentState must be valid");
    const StrategyInput* strategyInput = schedulerState->currentState->strategyInput;
    if (strategyInput->fChannels  < 1)   throw wns::Exception("Need at least one subBand");
    if (strategyInput->slotLength < 0.0) throw wns::Exception("Invalid slotLength");
    if (strategyInput->maxBeams   < 1)   throw wns::Exception("Need at least one beam");
    MESSAGE_SINGLE(NORMAL, logger, "Strategy::doStartScheduling(): old interface. Please overload and use new interface in strategies");
    assure(!isNewStrategy(),"default Strategy::doStartScheduling() only allowed for the old obsolete strategies");
    // call old interface (for convenience)
    this->doStartScheduling(strategyInput->fChannels, strategyInput->maxBeams, strategyInput->slotLength);
    //schedulingMapReady(strategyResult); // make the callBacks
    // An empty result for schedulingMap is returned.
    // That's ok because old implementations do not expect it.
    // They rely on the callBack mechanism.
    // The "bursts" have been updated via bursts_push_back() in the old strategies
    MapInfoCollectionPtr bursts = schedulerState->currentState->bursts;
    //MESSAGE_SINGLE(NORMAL, logger,"(old) doStartScheduling() done: bursts="<<bursts.getPtr()<<"="<<wns::scheduler::printMapInfoCollection(bursts)); // debug
    //AntennaPatternsPerUser& antennaPatterns = schedulerState->currentState->getGrouping()->patterns;
    if (schedulerState->currentState->groupingIsValid()) {
        wns::scheduler::GroupingPtr sdmaGrouping = schedulerState->currentState->getGrouping();
        StrategyResult strategyResult(schedulingMap,bursts,sdmaGrouping);
        return strategyResult;
    } else { // no SDMA grouping
        StrategyResult strategyResult(schedulingMap,bursts);
        return strategyResult;
    }
} // doStartScheduling backward compatibility wrapper

void
Strategy::doStartScheduling(int fChannels, int maxBeams, simTimeType slotLength)
{
    throw wns::Exception("Strategy::doStartScheduling() is old interface and must be overloaded. Better use new interface");
}

// NEW interface instead of compoundReady. Calls callBack functions of system resourceSchedulers (my caller)
void
Strategy::schedulingMapReady(StrategyResult& strategyResult)
{
     assure(schedulerState->currentState!=RevolvingStatePtr(),"currentState must be valid");
     // nothing to do if no callBack registered:
     if (schedulerState->currentState->strategyInput->callBackObject==NULL) return; // nothing to do
     assure(schedulerState->currentState->strategyInput->callBackObject!=NULL,"invalid callback");
//     MESSAGE_SINGLE(NORMAL, logger, "schedulingMapReady(): performing "<<strategyResult.bursts->size()<<" callbacks:");
//     if (strategyResult.bursts->size()==0) return; // nothing to do
//
//     //master is assumed
//     if(isTx()){ //is DL master ?!
//     SchedulingMapPtr schedulingMap = strategyResult.schedulingMap; // just a smartPtr
//     schedulerState->currentState->strategyInput->callBackObject->callBack(schedulingMap);
//     } // is UL master ?!
//  else { // method 2: iterate through MapInfoCollectionPtr
//         //MapInfoCollectionPtr bursts = strategyResult.bursts;
//         // copy into state, because we are asked later via getMapInfo()
//         schedulerState->currentState->bursts = strategyResult.bursts;
//         MapInfoCollectionPtr bursts;
//         // In slave scheduler translate the bursts into one before processing:
//         if (schedulerState->schedulerSpot == wns::scheduler::SchedulerSpot::ULSlave())
//         { // SLAVE
//             MapInfoCollectionPtr scheduledBursts = schedulerState->currentState->bursts; // input
//             bursts = MapInfoCollectionPtr(new wns::scheduler::MapInfoCollection); // output
//             MESSAGE_SINGLE(NORMAL, logger,"schedulingMapReady(): ULSlave: scheduledBursts="<<scheduledBursts.getPtr()<<"="<<wns::scheduler::printMapInfoCollection(scheduledBursts)); // debug
//             assure(schedulerState->currentState->strategyInput->mapInfoEntryFromMaster != MapInfoEntryPtr(),"need masterBurst");
//             MapInfoEntryPtr burst = schedulerState->currentState->strategyInput->mapInfoEntryFromMaster; // only this one burst is valid
//             for ( MapInfoCollection::iterator iterBurst = scheduledBursts->begin();
//                   iterBurst != scheduledBursts->end(); ++iterBurst)
//             {
//                 MapInfoEntryPtr scheduledBurst = (*iterBurst);
//                 // For slave scheduling the StrategyInput contains a (one!) mapInfoEntryFromMaster which specifies one subchannel only
//                 // However, the substrategies altogether return a number of mapInfoEntries=bursts, which are all "copies" of the original SmartPtr mapInfoEntryFromMaster
//                 // But the first burst already contains all information. But its start-end values are wrong.
//                 // So better copy the contents of all scheduledBursts into the masterBurst
//                 MESSAGE_SINGLE(NORMAL, logger,"schedulingMapReady(): ULSlave: iterBurst="<<scheduledBurst.getPtr());
//                 burst->user = scheduledBurst->user; // switch to new receiver (RAP)
//                 for (wns::scheduler::CompoundList::iterator iter=scheduledBurst->compounds.begin(); iter!=scheduledBurst->compounds.end(); ++iter)
//                 { // forall compounds in burst
//                     wns::ldk::CompoundPtr compoundPtr = *iter;
//                     burst->compounds.push_back(compoundPtr); // copy into masterBurst
//                 }
//             } // forall bursts
//             bursts->push_back(burst); // only one burst (in slave mode)
//             //} else if (schedulerState->schedulerSpot = wns::scheduler::SchedulerSpot::ULMaster()) { // UL Master
//         } else { // Master
//             bursts = schedulerState->currentState->bursts;
//         }
//         MESSAGE_SINGLE(NORMAL, logger,"schedulingMapReady(): bursts="<<bursts.getPtr()<<"="<<wns::scheduler::printMapInfoCollection(bursts)); // debug
//
//         MapInfoEntryPtr lastBurst; // only for debugging
//         for ( MapInfoCollection::iterator iterBurst = bursts->begin();
//               iterBurst != bursts->end(); ++iterBurst)
//         {
//             //MESSAGE_SINGLE(NORMAL, logger,"schedulingMapReady(): iterBurst="<<iterBurst->getPtr()); // debug
//             MapInfoEntryPtr burst = (*iterBurst);
//             // unset members?
//             if (burst->frameNr<0) { burst->frameNr=schedulerState->currentState->strategyInput->frameNr; }
//             // "-inf dBm" must be handled (some strategies leave this incomplete):
//             if (burst->txPower==wns::Power()) {
//                 assure (schedulerState->powerCapabilities.nominalPerSubband!=wns::Power(),"undefined powerCapabilities.nominalPerSubband=="<<schedulerState->powerCapabilities.nominalPerSubband);
//                 burst->txPower=schedulerState->powerCapabilities.nominalPerSubband;
//             }
//             if (burst->txPower.get_mW()==0.0) {
//                 assure (schedulerState->powerCapabilities.nominalPerSubband!=wns::Power(),"undefined powerCapabilities.nominalPerSubband=="<<schedulerState->powerCapabilities.nominalPerSubband);
//                 burst->txPower=schedulerState->powerCapabilities.nominalPerSubband;
//             }
//             if (!burst->estimatedCandI.isValid()) { // only old strategies need this:
//                 burst->estimatedCandI = (schedulerState->isTx)?
//                     (colleagues.registry->estimateTxSINRAt(burst->user)) // Tx
//                     :
//                     (colleagues.registry->estimateRxSINROf(burst->user)); // Rx;
//                 // estimatedCandI is old (flat channel) method here. Not CQI.
//                 MESSAGE_SINGLE(NORMAL, logger,"schedulingMapReady(): warning: too late calculation of estimatedCandI="<<burst->estimatedCandI.toSINR());
//             }
//             MESSAGE_SINGLE(NORMAL, logger,"schedulingMapReady(): powerCapabilities.nominalPerSubband="<<schedulerState->powerCapabilities.nominalPerSubband);
//             MESSAGE_SINGLE(NORMAL, logger,"schedulingMapReady(): iterBurst="<<burst.getPtr()<<" vs lastBurst="<<lastBurst.getPtr());
//             MESSAGE_SINGLE(NORMAL, logger,"schedulingMapReady(): iterBurst="<<burst.getPtr()<<burst->toString());
//             assure(burst->txPower.get_dBm()>-40.0,"bad txPower="<<burst->txPower);
//             assure(burst->phyModePtr != wns::service::phy::phymode::PhyModeInterfacePtr(),"phyModePtr="<<burst->phyModePtr);
//             // this can happen if ULSlaveScheduler works on burst=mapInfoEntryFromMaster and subStrategies return multiple copies of that:
//             assure(burst.getPtr()!=lastBurst.getPtr(),"burst="<<burst.getPtr()<<" equals lastBurst="<<lastBurst.getPtr());
//             schedulerState->currentState->strategyInput->callBackObject->
//                 callBack(burst);
//             lastBurst=burst; // only for debugging
//         } // foreach burst
//     } // method 2
//     MESSAGE_SINGLE(NORMAL, logger, "schedulingMapReady(): done ("<<strategyResult.bursts->size()<<" callbacks/mapInfoEntries/bursts).");
} // schedulingMapReady

/*
// Attention: RemainingTxPower depends on user in case of RS-RX
wns::Power
Strategy::getRemainingTxPower(const wns::scheduler::SchedulingMapPtr schedulingMap, int timeSlot) const
{
    wns::Power totalPower = schedulerState->powerCapabilities.maxOverall;
    return schedulingMap->getRemainingPower(totalPower,timeSlot);
} // getRemainingTxPower
*/

MapInfoEntryPtr
Strategy::doAdaptiveResourceScheduling(RequestForResource& request,
                                       SchedulingMapPtr schedulingMap)
{
    assure(colleagues.dsastrategy  !=NULL,"dsastrategy==NULL");
    assure(colleagues.dsafbstrategy!=NULL,"dsafbstrategy==NULL");
    assure(colleagues.apcstrategy  !=NULL,"apcstrategy==NULL");
    assure(schedulerState->currentState!=RevolvingStatePtr(),"currentState must be valid");
    assure(schedulerState->currentState->strategyInput!=NULL,"strategyInput must be valid");
    int frameNr = schedulerState->currentState->strategyInput->getFrameNr();
    MESSAGE_SINGLE(NORMAL, logger,"doAdaptiveResourceScheduling("<<request.user->getName()<<",cid="<<request.cid<<",bits="<<request.bits<<"): useCQI="<<schedulerState->useCQI);
    MapInfoEntryPtr resultMapInfoEntry; // empty means no result. Filled later

    // The slave RS-TX in the UT does not need to and cannot do AdaptiveResourceScheduling:
    if (schedulerState->powerControlType==PowerControlULSlave)
    {
        assure(schedulerState->schedulerSpot==wns::scheduler::SchedulerSpot::ULSlave(),
               "PowerControlULSlave requires SchedulerSpot::ULSlave");
        assure(schedulerState->currentState->strategyInput!=NULL,"need strategyInput");
        if (schedulerState->currentState->strategyInput->mapInfoEntryFromMaster != MapInfoEntryPtr())
        { // don't do anything. Just return the known=given masterBurst
            assure(schedulerState->currentState->strategyInput->mapInfoEntryFromMaster != MapInfoEntryPtr(),"need masterBurst");
            MapInfoEntryPtr mapInfoEntry = MapInfoEntryPtr(new MapInfoEntry(*(schedulerState->currentState->strategyInput->mapInfoEntryFromMaster))); // copy and new SmartPtr to carry the result
            assure(mapInfoEntry != MapInfoEntryPtr(),"need masterBurst");
            assure(mapInfoEntry->user != NULL,"need user in masterBurst");
            MESSAGE_SINGLE(NORMAL, logger,"doAdaptiveResourceScheduling(): using masterBurst in slave mode: user="<<mapInfoEntry->user->getName()<<" -> request.user="<<request.user->getName()<<" (destination peer)");
            mapInfoEntry->user = request.user; // switch to new receiver (RAP)
            assure(mapInfoEntry->phyModePtr!=wns::service::phy::phymode::PhyModeInterfacePtr(),"phyMode must be defined in masterBurst"<<mapInfoEntry->toString());
            request.phyModePtr=mapInfoEntry->phyModePtr; // needed later
            if (!schedulingMap->pduFitsInto(request,mapInfoEntry)) // no space
                return resultMapInfoEntry; // empty means no result
            return mapInfoEntry;
        } else {
            MESSAGE_SINGLE(NORMAL, logger,"doAdaptiveResourceScheduling(): using inputSchedulingMap. request.user="<<request.user->getName()<<" (destination peer)");
            assure(schedulerState->currentState->strategyInput->inputSchedulingMap != wns::scheduler::SchedulingMapPtr(), "slave scheduling requires inputSchedulingMap");
            assure(!colleagues.dsastrategy->requiresCQI(),"SlaveScheduler DSAStrategy must not require CQI");
            // go on with standard algorithm...
        }
    }
    //assure(schedulerState->powerControlType!=PowerControlULSlave,"cannot doAdaptiveResourceScheduling in slave scheduler");
    //assure(schedulerState->schedulerSpot!=wns::scheduler::SchedulerSpot::ULSlave(),
    //       "cannot doAdaptiveResourceScheduling in slave scheduler. Please switch powerControlSlave=True");

    ChannelQualitiesOnAllSubBandsPtr cqiForUser; // SmartPtr (allocated in CQI)
    wns::CandI estimatedCandI;

    assure(schedulerState->defaultPhyModePtr == wns::service::phy::phymode::PhyModeInterfacePtr(),
           "defaultPhyModePtr must not be set at this point (either master scheduler or inputSchedulingMap method)");

    if (schedulerState->useCQI)
    {
        assure (schedulerState->powerControlType!=PowerControlULSlave,"slave must not use CQI");
        // is the CQI state already available in our state?
        if (!schedulerState->currentState->channelQualitiesOfAllUsers->knowsUser(request.user))
        { // NO -> get current CQI state
            if (schedulerState->isTx)
            { // RS-TX (master DL)
                // TODO: change CQIHandler so that it produces SmartPtr
                cqiForUser = colleagues.registry->getChannelQualities4UserOnDownlink(request.user, frameNr);
            } else { // RS-RX (master UL)
                cqiForUser = colleagues.registry->getChannelQualities4UserOnUplink(request.user, frameNr);
            }
            //assure(cqiForUser!=ChannelQualitiesOnAllSubBandsPtr(), "cqiForUser["<<request.user->getName()<<"]==NULL");
            // just a SmartPtr copy:
            (*(schedulerState->currentState->channelQualitiesOfAllUsers)).insert(std::map<UserID,ChannelQualitiesOnAllSubBandsPtr>::value_type(request.user,cqiForUser));

        } else {
            cqiForUser = schedulerState->currentState->channelQualitiesOfAllUsers->find(request.user)->second;
        }
        //assure(schedulerState->currentState->channelQualitiesOfAllUsers->count(request.user) > 0, "channelQualitiesOfAllUsers["<<request.user->getName()<<"] empty");
        //assure(schedulerState->currentState->channelQualitiesOfAllUsers->knowsUser(request.user), "channelQualitiesOfAllUsers["<<request.user->getName()<<"] empty");
        // ^ at this point the CQI info may be empty (e.g. start of simulation).
        // This is checked later with CQIavailable.
    } else {
        // without CQI we must use old method "CandI estimateTxSINRAt" of registryProxy
        // done later because this is also fallback solution if CQI is not yet available
    }
    // sometimes we can not get the channel qualities,
    // like: due to some specifical design of WNS, BS may need to do scheduling for RN without sending or receiving any PDU from it

    dsastrategy::DSAResult dsaResult;
    int subChannel = dsastrategy::DSAsubChannelNotFound;
    int timeSlot = 0; // TDMA
    int beam = 0; // MIMO
    bool CQIrequired = colleagues.dsastrategy->requiresCQI();
    bool CQIavailable = (cqiForUser!=ChannelQualitiesOnAllSubBandsPtr())
        && (schedulerState->currentState->channelQualitiesOfAllUsers != ChannelQualitiesOfAllUsersPtr())
        && ((*(schedulerState->currentState->channelQualitiesOfAllUsers))[request.user]->size() > 0);

    MESSAGE_SINGLE(NORMAL, logger,"doAdaptiveResourceScheduling("<<request.user->getName()<<",cid="<<request.cid<<",bits="<<request.bits<<"): useCQI="<<schedulerState->useCQI<<",CQIrequired="<<CQIrequired<<",CQIavailable="<<CQIavailable);
    ChannelQualityOnOneSubChannel& cqiOnSubChannel
        = request.cqiOnSubChannel; // memory of request structure
    // start dynamic subchannel assignment (DSA):
    // with or without CQI information?
    if (schedulerState->useCQI && CQIrequired && CQIavailable)
    {
        assure (schedulerState->powerControlType!=PowerControlULSlave,"slave must not use CQI");
        MESSAGE_SINGLE(NORMAL, logger,"doAdaptiveResourceScheduling("<<request.user->getName()<<",cid="<<request.cid<<",bits="<<request.bits<<"): calling dsastrategy");
        // start dynamic subchannel assignment
        dsaResult = colleagues.dsastrategy->getSubChannelWithDSA(request, schedulerState, schedulingMap);
        subChannel = dsaResult.subChannel;
        timeSlot   = dsaResult.timeSlot;
        beam       = dsaResult.beam;
        if (subChannel==dsastrategy::DSAsubChannelNotFound)
            return resultMapInfoEntry; // empty means no result
        assure(cqiForUser!=ChannelQualitiesOnAllSubBandsPtr(), "cqiForUser["<<request.user->getName()<<"]==NULL");
        // this writes into request structure:
        cqiOnSubChannel = (*cqiForUser)[subChannel]; // copy
        assure(&cqiOnSubChannel == &(request.cqiOnSubChannel),"copy failed: addresses don't match");
        estimatedCandI.I = cqiOnSubChannel.interference;
        MESSAGE_SINGLE(NORMAL, logger,"doAdaptiveResourceScheduling("<<request.user->getName()<<",cid="<<request.cid<<",bits="<<request.bits<<"): subChannel="<<subChannel);
    } else { // no CQI required or available
        if (!CQIrequired) {
            MESSAGE_SINGLE(NORMAL, logger,"doAdaptiveResourceScheduling(): calling dsastrategy");
            // start dynamic subchannel assignment
            dsaResult = colleagues.dsastrategy->getSubChannelWithDSA(request, schedulerState, schedulingMap);
        } else { // no CQI available (principially or temporary)
            MESSAGE_SINGLE(NORMAL, logger,"doAdaptiveResourceScheduling(): calling dsafbstrategy (fallback)");
            dsaResult = colleagues.dsafbstrategy->getSubChannelWithDSA(request, schedulerState, schedulingMap);
        }
        subChannel = dsaResult.subChannel;
        timeSlot   = dsaResult.timeSlot;
        beam       = dsaResult.beam;
        if (subChannel==dsastrategy::DSAsubChannelNotFound)
            return resultMapInfoEntry; // empty means no result
        // assume flat channel and nominal TxPower for this case
        wns::Power nominalPowerPerSubChannel = getPowerCapabilities(request.user).nominalPerSubband;
        estimatedCandI = (schedulerState->isTx)?
            (colleagues.registry->estimateTxSINRAt(request.user)) // Tx
            :
            (colleagues.registry->estimateRxSINROf(request.user)); // Rx
        // this writes into request structure:
        cqiOnSubChannel.interference = estimatedCandI.I;
        cqiOnSubChannel.pathloss = nominalPowerPerSubChannel / estimatedCandI.C;
    } // with|without CQI information
    // Tell result of DSA: subChannel
    MESSAGE_SINGLE(NORMAL, logger,"doAdaptiveResourceScheduling("<<request.user->getName()<<",cid="<<request.cid<<",bits="<<request.bits<<"):"
                   <<" subChannel.tSlot.beam="<<subChannel<<"."<<timeSlot<<"."<<beam);

    if (subChannel==dsastrategy::DSAsubChannelNotFound)
        return resultMapInfoEntry; // empty means no result
    // must be replaced by code which copes with that case:
    assure(subChannel!=dsastrategy::DSAsubChannelNotFound,"DSAsubChannelNotFound");

    // fix the proposed subChannel value:
    request.subChannel = subChannel;
    request.timeSlot   = timeSlot;
    request.beam       = beam;

    wns::Power txPower;
    apcstrategy::APCResult apcResult;
    /** @todo: instead of this if-clause we could also use the "APCSlave" strategy instead */
    if (schedulerState->powerControlType!=PowerControlULSlave)
    { // Master
        // do adaptive power allocation
        apcResult = colleagues.apcstrategy->doStartAPC(request, schedulerState, schedulingMap);
        // not (yet) possible: if (?==apcstrategy::APCNotFound) return resultMapInfoEntry; // empty means no result
        if (apcResult.txPower == wns::Power())
            return resultMapInfoEntry; // empty means no result
        // fix the proposed phyMode value:
        request.phyModePtr = apcResult.phyModePtr;
        txPower = apcResult.txPower;
        // process result:
        estimatedCandI.C = apcResult.txPower / cqiOnSubChannel.pathloss.get_factor(); // "- in dB" = "/ in Power"
        MESSAGE_SINGLE(NORMAL, logger,"doAdaptiveResourceScheduling(): txP="<<apcResult.txPower<<", pl="<<cqiOnSubChannel.pathloss<<", sinr="<<estimatedCandI.toSINR()<<", PhyMode="<<*(apcResult.phyModePtr));
        assure(std::fabs(estimatedCandI.C.get_dBm()-apcResult.estimatedCandI.C.get_dBm())<1e-6,"estimatedCandI mismatch: C="<<estimatedCandI.C<<" != "<<apcResult.estimatedCandI.C);
        assure(std::fabs(estimatedCandI.I.get_dBm()-apcResult.estimatedCandI.I.get_dBm())<1e-6,"estimatedCandI mismatch: I="<<estimatedCandI.I<<" != "<<apcResult.estimatedCandI.I);
        assure(std::fabs(estimatedCandI.toSINR().get_dB()-apcResult.sinr.get_dB())<1e-6,"sinr mismatch: sinr="<<estimatedCandI.toSINR()<<" != "<<apcResult.sinr);
        // estimatedCandI (calculated here) and apcResult.estimatedCandI should be the same
        double minSINRforPhyMode = colleagues.registry->getPhyModeMapper()->getMinimumSINR();
        if ((apcResult.sinr.get_dB() < minSINRforPhyMode))
        { // this could mean "APC failed"
            MESSAGE_SINGLE(NORMAL, logger,"doAdaptiveResourceScheduling(): too low SINR! sinr="<<estimatedCandI.toSINR()<<", PhyMode="<<*(apcResult.phyModePtr)<<" requires "<<minSINRforPhyMode<<"dB");
            // ^ this may happen in the early milliseconds of a simulation / after association,
            // when there is no realistic estimatedCandI and CQI.
            // If we would break here in this case, there will never be a transmission.
            // which criterion? cqiOnSubChannel.pathloss.get_dB() > 150 ?

            // If the value is extremely low, it is likely that something bad has happened
            assure(estimatedCandI.C.get_dBm() > -190,
                   "sinr="<<apcResult.sinr.get_dB()<<" but minSINRforPhyMode="<<minSINRforPhyMode<<" not reached. Estimation was: C="<<estimatedCandI.C<<", I="<<estimatedCandI.I<<" (blind)");
            if (schedulerState->excludeTooLowSINR)
            {
                return resultMapInfoEntry; // empty means no result
            }
        }
    } else { // slave scheduling (PowerControlULSlave)
        request.phyModePtr = schedulingMap->getPhyModeUsedInResource(subChannel,timeSlot,beam);
        txPower = schedulingMap->getTxPowerUsedInResource(subChannel,timeSlot,beam);
    }

    // This really fixes the proposed values of DSA and APC.
    resultMapInfoEntry = MapInfoEntryPtr(new MapInfoEntry());
    resultMapInfoEntry->frameNr    = frameNr;
    resultMapInfoEntry->subBand    = subChannel;
    resultMapInfoEntry->timeSlot   = timeSlot;
    resultMapInfoEntry->beam       = beam;
    resultMapInfoEntry->user       = request.user;
    resultMapInfoEntry->txPower    = txPower; // apcResult.txPower;
    resultMapInfoEntry->phyModePtr = request.phyModePtr; // = apcResult.phyModePtr
    resultMapInfoEntry->estimatedCandI = estimatedCandI; // ?= apcResult.estimatedCandI;
    // Set antennaPattern according to grouping result
    if (groupingRequired()) {
        assure(schedulerState->currentState->getGrouping() != GroupingPtr(),"invalid grouping");
        wns::service::phy::ofdma::PatternPtr antennaPattern = schedulerState->currentState->getGrouping()->patterns[request.user];
        //Group currentGroup = schedulerState->currentState->grouping->groups[currentGroupIndex?];
        //wns::CandI estimatedCandI = currentGroup[request.user];
        //assure(estimatedCandI==apcResult.estimatedCandI,"estimatedCandI mismatch");
        resultMapInfoEntry->pattern = antennaPattern;
    }
    // else:
    // what about static antenna patterns (e.g. for sectorized transmission)?
    // can we set them here or is this done statically?
    // what about PhyUser::deleteReceiveAntennaPatterns() then? (called every frame when switching Rx/Tx in FDD mode)
    // Does this flatten every pattern ever set?

    //resultMapInfoEntry->compounds = (empty list) // not here, but in caller
    /* we don't put the compound(s) in here because
       a) we don't have it/them here
       b) we don't know how many the packet scheduling strategy wants to put in
       c) request.bits only asked for the first PDU but more may fit in.
       d) with "dynamic segmentation" the packet is segmented after deciding PhyMode=>duration
       => we don't know start and end time here
    */
    //double dataRate = apcResult.phyModePtr->getDataRate();
    //simTimeType compoundDuration = request.bits / dataRate;
    //resultMapInfoEntry->compoundDuration = compoundDuration; // done later
    resultMapInfoEntry->start = wns::scheduler::undefinedTime;
    resultMapInfoEntry->end   = wns::scheduler::undefinedTime;
    return resultMapInfoEntry;
} // doAdaptiveResourceScheduling

// OLD interface (to satisfy the old strategies)
void
Strategy::compoundReady(unsigned int fSlot,
                        simTimeType startTime, simTimeType endTime, UserID user,
                        const wns::ldk::CompoundPtr& pdu, unsigned int beam,
                        wns::service::phy::ofdma::PatternPtr pattern,
                        MapInfoEntryPtr burst,
                        const wns::service::phy::phymode::PhyModeInterface& phyMode,
                        wns::Power requestedTxPower,
                        wns::CandI estimatedCandI)
{
    assure(!isNewStrategy(),"compoundReady() only allowed for the old obsolete strategies");
    return; // callBack now done at the end in schedulingMapReady()
} // compoundReady


/** @brief helper method to support the old scheduler strategies.
    Replaces
    bursts.push_back(currentBurst);
    by
    bursts_push_back(currentBurst);
*/
void
Strategy::bursts_push_back(MapInfoEntryPtr burst)
{
    assure(schedulerState->currentState!=RevolvingStatePtr(),"currentState must be valid");
    assure(schedulerState->currentState->bursts!=MapInfoCollectionPtr(),"bursts must be valid");
    MESSAGE_SINGLE(NORMAL, logger,"bursts_push_back(#"<<schedulerState->currentState->bursts->size()+1<<"): "<<burst.getPtr());
    assure(!isNewStrategy(),"bursts_push_back() only allowed for the old obsolete strategies");
    assure(burst->end > burst->start,"burst->start="<<burst->start<<" and burst->end="<<burst->end);
    schedulerState->currentState->bursts->push_back(burst);
}

/** @brief helper method to support the old scheduler strategies.
    Replaces bursts.back()->compounds.push_back(pdu);
    by
    bursts_push_back_compound(pdu);
*/
void
Strategy::bursts_push_back_compound(wns::ldk::CompoundPtr pdu)
{
    //MESSAGE_SINGLE(NORMAL, logger,"bursts_push_back_compound()");
    assure(schedulerState->currentState!=RevolvingStatePtr(),"currentState must be valid");
    assure(schedulerState->currentState->bursts!=MapInfoCollectionPtr(),"bursts must be valid");
    MESSAGE_SINGLE(NORMAL, logger,"bursts_push_back_compound(#"<<schedulerState->currentState->bursts->back()->compounds.size()+1<<")");
    assure(!isNewStrategy(),"bursts_push_back_compound() only allowed for the old obsolete strategies");
    schedulerState->currentState->bursts->back()->compounds.push_back(pdu);
}

/** @brief helper method to support the old scheduler strategies. */
bool
Strategy::isEirpLimited()
{
    assure(!isNewStrategy(),"isEirpLimited() only allowed for the old obsolete strategies");
    return schedulerState->eirpLimited;
}

/** @brief helper method to support the old scheduler strategies. */
int
Strategy::getNumBursts() const
{
    assure(schedulerState->currentState!=RevolvingStatePtr(),"currentState must be valid");
    assure(schedulerState->currentState->bursts!=MapInfoCollectionPtr(),"bursts must be valid");
    return schedulerState->currentState->bursts->size();
}

wns::service::phy::phymode::PhyModeInterfacePtr
Strategy::getBestPhyMode(const wns::Ratio& sinr) const
{
    if (schedulerState->currentState->strategyInput->defaultPhyModePtr != wns::service::phy::phymode::PhyModeInterfacePtr()) {
        assure(schedulerState->currentState->strategyInput->defaultPhyModePtr->isValid(),"defaultPhyModePtr is invalid!");
        return schedulerState->currentState->strategyInput->defaultPhyModePtr;
    } else {// do link adaptation
        return colleagues.registry->getBestPhyMode(sinr);
    }
}

wns::scheduler::SchedulerSpotType
Strategy::getSchedulerSpotType() const {
    assure(schedulerState!=SchedulerStatePtr(),"schedulerState must be valid");
    assure(schedulerState->schedulerSpot>0,"uninitialized schedulerSpot="<<schedulerState->schedulerSpot);
    return schedulerState->schedulerSpot;
}

bool
Strategy::isTx() const
{
    // comes from Python. isTx=true for RS-TX (DL,UL) in (BS,UT,RN)
    assure(schedulerState!=SchedulerStatePtr(),"schedulerState must be valid");
    return schedulerState->isTx; // txMode;
}

/** @brief helper method to support the old scheduler strategies. Obsolete. */
void
Strategy::clearMap()
{
    assure(schedulerState->currentState!=RevolvingStatePtr(),"currentState must be valid");
    assure(schedulerState->currentState->bursts!=MapInfoCollectionPtr(),"bursts must be valid");
    schedulerState->currentState->bursts = MapInfoCollectionPtr(); // NULL pointer
}

// method only to support the old strategies. Not intended for new schedulers.
wns::Power
Strategy::getTxPower() const
{
    wns::Power power;
    MESSAGE_SINGLE(NORMAL, logger, "getTxPower()");
    // return the value commanded by the powercontrol
    assure(schedulerState!=SchedulerStatePtr(),"schedulerState must be valid");
    assure(schedulerState->currentState!=RevolvingStatePtr(),"currentState must be valid");
    assure(schedulerState->currentState->strategyInput!=NULL,"need strategyInput");
    assure(!isNewStrategy(),"getTxPower() only allowed for the old obsolete strategies");
    if (schedulerState->powerCapabilities.nominalPerSubband==wns::Power())
    { // not yet prepared. Do it now.
        MESSAGE_SINGLE(NORMAL, logger, "getTxPower: asking getPowerCapabilities(NULL)...");
        getPowerCapabilities(NULL); // NULL because peer unknown
    }
    power = schedulerState->powerCapabilities.nominalPerSubband;
    assure(power!=wns::Power(),"undefined power="<<power);
    return power;
}

float
Strategy::getResourceUsage() const
{
    assure(schedulerState!=SchedulerStatePtr(),"schedulerState must be valid");
    assure(schedulerState->currentState!=RevolvingStatePtr(),"currentState must be valid");
    assure(schedulerState->currentState->schedulingMap!=wns::scheduler::SchedulingMapPtr(),"schedulingMap must be valid");
    // This is the default implementation. Overload in your desired strategy
    // if you want information about the resource usage.
    // In general it is better to call schedulingMap->getResourceUsage() in the system specific methods after scheduling
    if (schedulerState->currentState->schedulingMap!=SchedulingMapPtr()) {
        return schedulerState->currentState->schedulingMap->getResourceUsage();
    } else {
        return 0.0;
    }
}

bool
Strategy::groupingRequired() const
{
    assure(schedulerState!=SchedulerStatePtr(),"schedulerState must be valid");
    assure(schedulerState->currentState!=RevolvingStatePtr(),"currentState must be valid");
    assure(schedulerState->currentState->strategyInput!=NULL,"schedulerState->currentState->strategyInput must be valid");
    return ((schedulerState->currentState->strategyInput->beamforming)
            && (schedulerState->currentState->strategyInput->getMaxBeams()>1))
        ? true:false;
}
