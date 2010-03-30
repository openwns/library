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

#include <WNS/scheduler/strategy/staticpriority/SubStrategyInterface.hpp>
#include <WNS/scheduler/strategy/StaticPriority.hpp>
#include <WNS/scheduler/strategy/Strategy.hpp>
#include <WNS/scheduler/strategy/SchedulerState.hpp>
#include <WNS/scheduler/SchedulingMap.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>
#include <WNS/pyconfig/View.hpp>

#include <vector>
#include <map>
#include <algorithm>
#include <iostream>
#include <set>

using namespace wns::scheduler;
using namespace wns::scheduler::strategy;
using namespace wns::scheduler::strategy::staticpriority;

STATIC_FACTORY_REGISTER_WITH_CREATOR(StaticPriority,
                                     StrategyInterface,
                                     "StaticPriority",
                                     wns::PyConfigViewCreator);

StaticPriority::StaticPriority(const wns::pyconfig::View& config):
    Strategy(config),
    numberOfPriorities(0),
    subStrategies(),
    resourceUsage(0.0)
{
    MESSAGE_SINGLE(NORMAL, logger, "StaticPriority() instance created.");
}

StaticPriority::~StaticPriority()
{
    for (int priority=0; priority<numberOfPriorities; ++priority) {
        if (subStrategies[priority]!=NULL) { delete subStrategies[priority]; }
    }
}

float
StaticPriority::getResourceUsage() const
{
    return resourceUsage;
}

// inherited by base class Strategy to do first-time initialization:
void
StaticPriority::onColleaguesKnown()
{
    Strategy::onColleaguesKnown(); // must be done in every derived method
    numberOfPriorities = colleagues.registry->getNumberOfPriorities();

    // loop over all priorities and initialize subStrategies
    assure(numberOfPriorities == pyConfig.len("subStrategies"),"numberOfPriorities="<<numberOfPriorities<<" != "<<pyConfig.len("subStrategies"));
    MESSAGE_SINGLE(NORMAL, logger,"StaticPriority::onColleaguesKnown(), numberOfPriorities="<<numberOfPriorities);
    for (int priority=0; priority<numberOfPriorities; ++priority) {
        wns::pyconfig::View substrategyView = pyConfig.getView("subStrategies",priority);
        std::string substrategyName = substrategyView.get<std::string>("__plugin__");
        MESSAGE_SINGLE(NORMAL, logger, "subStrategy["<<priority<<"]="<<substrategyName);
        wns::scheduler::strategy::staticpriority::SubStrategyInterface* substrategy = NULL;
        if (substrategyName.compare("NONE")!=0) {
            // create the subscheduling strategy for this priority:
            wns::scheduler::strategy::staticpriority::SubStrategyCreator* subStrategyCreator = wns::scheduler::strategy::staticpriority::SubStrategyFactory::creator(substrategyName);
            substrategy = subStrategyCreator->create(substrategyView);
        } else {
            substrategy = NULL;
        }
        subStrategies.push_back(substrategy);
    } // for all priorities

    SchedulerStatePtr schedulerState = getSchedulerState();
    // priority is out of [0..MaxPriority-1]:
    for (int priority=0; priority<numberOfPriorities; ++priority) {
        MESSAGE_SINGLE(NORMAL, logger, "initializing subStrategy["<<priority<<"]");
        wns::scheduler::strategy::staticpriority::SubStrategyInterface* substrategy = subStrategies[priority];
        assure(substrategy!=NULL, "The substrategy["<<priority<<"] is not accessible");
        substrategy->setColleagues(this, colleagues.queue, colleagues.registry, colleagues.harq);
        //schedulerState->currentSubStrategyState = ...;
    }
}

StrategyResult
StaticPriority::doStartScheduling(SchedulerStatePtr schedulerState,
                                  SchedulingMapPtr schedulingMap)
{
    assure(numberOfPriorities>0,"illegal numberOfPriorities="<<numberOfPriorities);
    int frameNr = schedulerState->currentState->strategyInput->getFrameNr();
    MESSAGE_SINGLE(NORMAL, logger, "StaticPriority::doStartScheduling(frame="<<frameNr<<"):"
                   << " numberOfPriorities="<<numberOfPriorities);

    // prepare result datastructure:
    MapInfoCollectionPtr mapInfoCollection = MapInfoCollectionPtr(new wns::scheduler::MapInfoCollection);
    StrategyResult strategyResult(schedulingMap,mapInfoCollection);
    this->resourceUsage=0.0;

    MESSAGE_SINGLE(NORMAL, logger, "doStartScheduling(): userSelection. getDL=" << schedulerState->isDL << ", isTX=" << schedulerState->isTx << ", schedulerSpot="<<wns::scheduler::SchedulerSpot::toString(schedulerState->schedulerSpot));

    // user selection
    UserSet allUsers;
    //if ( !(schedulerState->isDL) && !schedulerState->isTx )
    /*
    if ( schedulerState->schedulerSpot == wns::scheduler::SchedulerSpot::ULMaster() )
    {       // I am master scheduler for uplink (RS-RX)
        allUsers = colleagues.registry->getActiveULUsers();
        //allUsers = colleagues.queue->getQueuedUsers(); // soon possible with new queueInterface [rs]
        MESSAGE_SINGLE(NORMAL, logger, "doStartScheduling(): Master Rx-Scheduling...");
    } else {
        // get all users which have PDUs in their queues
        allUsers = colleagues.queue->getQueuedUsers();
        MESSAGE_SINGLE(NORMAL, logger, "doStartScheduling(): Slave UL-Scheduling or Master DL-Scheduling...");
    }
    */
    // the same for UL/DL:
    //allUsers = colleagues.queue->getQueuedUsers();
    //MESSAGE_SINGLE(NORMAL, logger, "doStartScheduling(): allUsers.size()="<<allUsers.size()<<": Users="<<printUserSet(allUsers));
    // filter reachable users
    //UserSet activeUsers = colleagues.registry->filterReachable(allUsers,frameNr);
    //MESSAGE_SINGLE(NORMAL, logger, "doStartScheduling(): activeUsers.size()="<<activeUsers.size()<<": Users="<<printUserSet(activeUsers));

    // prepare grouping here before going into priorities (subschedulers).
    // This code block could also be moved into the base class Strategy::startScheduling()
    if (groupingRequired() && !colleagues.queue->isEmpty()) // only if (maxSpatialLayers>1)
    {   // grouping needed for beamforming & its antenna pattern
        GroupingPtr sdmaGrouping = schedulerState->currentState->getNewGrouping(); // also stored in schedulerState
        int maxSpatialLayers = schedulerState->currentState->strategyInput->maxSpatialLayers;
        allUsers = colleagues.queue->getQueuedUsers();
        UserSet activeUsers = colleagues.registry->filterReachable(allUsers,frameNr);
        if ( schedulerState->isTx ) // transmitter grouping
            sdmaGrouping = colleagues.grouper->getTxGroupingPtr(activeUsers, maxSpatialLayers);
        else // receiver grouping
            sdmaGrouping = colleagues.grouper->getRxGroupingPtr(activeUsers, maxSpatialLayers);
        assure(schedulerState->currentState->getGrouping() == sdmaGrouping,"invalid grouping");
        // ^ otherwise we have to set it here.
        MESSAGE_SINGLE(NORMAL, logger, "doStartScheduling(): Number of Groups = " << sdmaGrouping->groups.size());
        MESSAGE_SINGLE(NORMAL, logger, "doStartScheduling(): grouping.getDebugOutput = " << sdmaGrouping->getDebugOutput());
        strategyResult.sdmaGrouping = sdmaGrouping; // set grouping into result output (needed later to set antennaPatterns)
    } else {
        MESSAGE_SINGLE(VERBOSE, logger, "doStartScheduling(): no grouping required.");
    }
    //if ( !activeUsers.empty() ) { // NO! go into all subStrategies anytime
    //ConnectionAttributes connectionAttributes; // NEW: std::map< ConnectionID, ConnectionsCharacteristics >

    // start scheduling with highest priority
    // a connection with p=0 (undefined) must not exist
    //assure(colleagues.registry->getConnectionsForPriority(0).size()==0,"getConnectionsForPriority(0)>0 ???");
    // priority is out of [0..MaxPriority-1]:
    for ( int priority = 0; priority < numberOfPriorities; ++priority )
    {
        // (grouping alternative: do grouping within each priority; do be discussed; but seems to be less useful)
        if (subStrategies[priority] == NULL) continue;
        schedulerState->currentState->setCurrentPriority(priority);
        MESSAGE_SINGLE(NORMAL, logger, "doStartScheduling(): now scheduling priority=" << priority);
        // get all registered connections for the current priority
        ConnectionSet allConnections = colleagues.registry->getConnectionsForPriority(priority); // all
        MESSAGE_SINGLE(NORMAL, logger, "allConnections      = "<<printConnectionSet(allConnections));
        ConnectionSet reachableConnections = colleagues.registry->filterReachable(allConnections,frameNr);
        MESSAGE_SINGLE(NORMAL, logger, "reachableConnections= "<<printConnectionSet(reachableConnections));
        // don't filter out unqueued cids since the subStrategy may want to update the state for every cid
        //ConnectionSet activeConnections = colleagues.queue->filterQueuedCids(reachableConnections);
        //MESSAGE_SINGLE(NORMAL, logger, "activeConnections   ="<<printConnectionSet(activeConnections));
        schedulerState->currentState->activeConnections = reachableConnections;
        // start SubScheduling in any case (even with empty user or cid list)
        // because the substrategies may need to keep and track their own state over time
        MapInfoCollectionPtr resultBursts = subStrategies[priority]->doStartSubScheduling(schedulerState, schedulingMap);
        // copy ^ of std::list<MapInfoEntryPtr>
        if (resultBursts->size()>0) {
            MESSAGE_SINGLE(NORMAL, logger, "merged "<<resultBursts->size()<<" entries of resultBursts="<<resultBursts.getPtr()<<" into mapInfoCollection="<<mapInfoCollection.getPtr()<<" (now size="<<mapInfoCollection->size()<<")");
            mapInfoCollection->join(*resultBursts); // collects result bursts. Do not use merge! (sorts bySmartPtr)
        }
    } // end for (over all priorities)
    // READY!
    MESSAGE_SINGLE(NORMAL, logger, "StaticPriority: "<<schedulingMap->getNumberOfCompounds()<<" compounds scheduled");
    MESSAGE_SINGLE(NORMAL, logger, schedulingMap->toString());
    this->resourceUsage = schedulingMap->getResourceUsage();
    /*
      #ifndef WNS_NDEBUG
      MESSAGE_SINGLE(NORMAL, logger, "mapInfoCollection(method1)="<<wns::scheduler::printMapInfoCollection(mapInfoCollection));
      MapInfoCollectionPtr mapInfoCollection_method2 = MapInfoCollectionPtr(new wns::scheduler::MapInfoCollection);
      // translate result into currentBurst to allow bursts.push_back(currentBurst)
      schedulingMap->convertToMapInfoCollection(mapInfoCollection_method2);
      MESSAGE_SINGLE(NORMAL, logger, "mapInfoCollection(method2)="<<wns::scheduler::printMapInfoCollection(mapInfoCollection));
      #endif
    */
    return strategyResult;
} // doStartScheduling()
