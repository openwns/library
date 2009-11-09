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
    MESSAGE_SINGLE(NORMAL, logger, "HARQRetransmission::getValidCurrentConnection");
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
    MESSAGE_SINGLE(NORMAL, logger, "HARQRetransmission::getNextConnection");
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
    // harq colleague may be NULL, e.g. Uplink Master Scheduler does not have HARQ
    if(colleagues.harq == NULL)
    {
        MapInfoCollectionPtr mapInfoCollection = MapInfoCollectionPtr(new wns::scheduler::MapInfoCollection);

        return mapInfoCollection;
    }

    wns::scheduler::SchedulingTimeSlotPtr timeslot = colleagues.harq->nextRetransmission();

    MESSAGE_SINGLE(NORMAL, logger, "Checking for retransmissions");
    while(timeslot != NULL)
    {
        for (wns::scheduler::SubChannelVector::iterator it = schedulingMap->subChannels.begin();
             it != schedulingMap->subChannels.end();
             ++it
            )
        {
            if (it->isEmpty())
            {
                MESSAGE_BEGIN(NORMAL, logger, m, "Retransmitting ");
                m << " for HARQ process " << timeslot->harq.processID;
                m << " sent on subchannel " << it->subChannelIndex;
                MESSAGE_END();

                /**
                 * @todo dbn: Maybe enable multiple temporalResources. Check destination users in uplink.
                 */
                it->temporalResources[0] = timeslot;
                it->temporalResources[0]->subChannelIndex = it->subChannelIndex;
                it->temporalResources[0]->physicalResources[0].subChannelIndex = it->subChannelIndex;
                break;
            }
        }
        timeslot = colleagues.harq->nextRetransmission();
    }

    MapInfoCollectionPtr mapInfoCollection = MapInfoCollectionPtr(new wns::scheduler::MapInfoCollection);

    return mapInfoCollection;
} // doStartSubScheduling

