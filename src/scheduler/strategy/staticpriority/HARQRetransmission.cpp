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

#include <WNS/scheduler/strategy/staticpriority/HARQRetransmission.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>

#include <vector>
#include <map>
#include <algorithm>
#include <iostream>

using namespace std;
using namespace wns::scheduler;
using namespace wns::scheduler::strategy;
using namespace wns::scheduler::strategy::staticpriority;

STATIC_FACTORY_REGISTER_WITH_CREATOR(HARQRetransmission,
                                     SubStrategyInterface,
                                     "HARQRetransmission",
                                     wns::PyConfigViewCreator);


HARQRetransmission::HARQRetransmission(const wns::pyconfig::View& config)
    : SubStrategy(config)
{
    blockSize=INT_MAX; // ExhaustiveRoundRobin=RoundRobin with infinite blockSize
    MESSAGE_SINGLE(NORMAL, logger, "HARQRetransmission(): constructed");
}

HARQRetransmission::~HARQRetransmission()
{
}

void
HARQRetransmission::initialize()
{
    // make state
    lastScheduledConnection = 0;
    //colleagues.harq->initialize(); // TODO!
    MESSAGE_SINGLE(NORMAL, logger, "HARQRetransmission(): initialized");
}

wns::scheduler::ConnectionID
HARQRetransmission::getValidCurrentConnection(const ConnectionSet &currentConnections, ConnectionID cid) const
{
    // uses state var currentConnections
    wns::scheduler::ConnectionSet::iterator iter =
        currentConnections.upper_bound(cid);
    if ( iter != currentConnections.end() ) { // exists
        return *iter;
    } else { // continue with next higher cid
        return *currentConnections.begin();
    }
    //MESSAGE_SINGLE(NORMAL, logger, "getValidCurrentConnection("<<cid<<") = ");
}

wns::scheduler::ConnectionID
HARQRetransmission::getNextConnection(const ConnectionSet &currentConnections, ConnectionID cid) const
{
    // uses state var currentConnections
    wns::scheduler::ConnectionSet::iterator iter =
        currentConnections.upper_bound(cid);
    if ( iter != currentConnections.end() ) { // found
        if ( *iter == cid ) { // go on
            iter++;
        } else {
            return *iter;
        }
    }
    if ( iter != currentConnections.end() ) { // exists
        return *iter;
    } else { // continue with next higher cid
        return *currentConnections.begin();
    }
    //MESSAGE_SINGLE(NORMAL, logger, "getNextConnection("<<cid<<") = ");
}

wns::scheduler::MapInfoCollectionPtr
HARQRetransmission::doStartSubScheduling(SchedulerStatePtr schedulerState,
                                         wns::scheduler::SchedulingMapPtr schedulingMap)
{
    MapInfoCollectionPtr mapInfoCollection = MapInfoCollectionPtr(new wns::scheduler::MapInfoCollection); // result datastructure
    ConnectionSet &currentConnections = schedulerState->currentState->activeConnections;
    if ( currentConnections.empty() ) return mapInfoCollection; // nothing to do
    wns::scheduler::ConnectionID currentConnection = getValidCurrentConnection(currentConnections,lastScheduledConnection);
    MESSAGE_SINGLE(NORMAL, logger, "HARQRetransmission::doStartSubScheduling("<<printConnectionSet(currentConnections)<<") start with cid="<<currentConnection);

    bool spaceLeft=true;
    while(spaceLeft)
    {
        int pduCounter = 0;
        // schedule #=blockSize PDUs for this CID here...
        // TODO: get info from colleagues.harq instead of queue:
        while( //colleagues.harq->queueHasPDUs(currentConnection) // TODO !!!
               //&&
               (pduCounter<blockSize)
               && spaceLeft)
        { // spaceLeft[currentConnection] to be more precise
            spaceLeft = scheduleCid(schedulerState,schedulingMap,currentConnection,pduCounter,blockSize,mapInfoCollection);
        } // while PDUs in queue
        // TODO: get info from colleagues.harq instead of queue:
        if (1)//(!colleagues.harq->queueHasPDUs(currentConnection)) // TODO !!!
        { // exit because of queue empty (most probable case for low traffic)
            currentConnections.erase(currentConnection);
            if (currentConnections.size()==0) break; // all queues empty
        }
        lastScheduledConnection = currentConnection; // this one really had pdus scheduled
        currentConnection = getNextConnection(currentConnections,currentConnection);
        MESSAGE_SINGLE(NORMAL, logger, "doStartSubScheduling(): next connection="<<currentConnection);
    } // while(spaceLeft)
    MESSAGE_SINGLE(NORMAL, logger, "doStartSubScheduling(): ready: mapInfoCollection="<<mapInfoCollection.getPtr()<<" of size="<<mapInfoCollection->size());
    return mapInfoCollection;
} // doStartSubScheduling

