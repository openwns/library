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
        MESSAGE_SINGLE(NORMAL, logger, "colleagues.harq==NULL is illegal. Please choose another strategy than HARQRetransmission");
        return mapInfoCollection;
    }
    assure(colleagues.harq!=NULL,"colleagues.harq==NULL is illegal. Please choose another strategy than HARQRetransmission");

    /**
     * @todo dbn/rs: Take reachability into account! Some target peers may be not reachable.
     */
    int numberOfRetransmissions = colleagues.harq->getNumberOfRetransmissions(/* @todo peer */);
    MESSAGE_SINGLE(NORMAL, logger, "doStartSubScheduling: "<<numberOfRetransmissions<<" HARQ retransmission(s) waiting");
    if (numberOfRetransmissions>0) {
        wns::scheduler::SchedulingTimeSlotPtr resourceBlock = colleagues.harq->nextRetransmission();
        assure(resourceBlock != NULL, "resourceBlock == NULL although numberOfRetransmissions="<<numberOfRetransmissions);
        while(resourceBlock != NULL)
        {
            bool foundSpace=false;
            // iterate over all subchannels (like Linear Frequency First DSA Strategy):
            for (wns::scheduler::SubChannelVector::iterator iterSubChannel = schedulingMap->subChannels.begin();
                 iterSubChannel != schedulingMap->subChannels.end();
                 ++iterSubChannel
                )
            {
                SchedulingSubChannel& subChannel = *iterSubChannel;
                int subChannelIndex = subChannel.subChannelIndex;
                for ( SchedulingTimeSlotPtrVector::iterator iterTimeSlot = subChannel.temporalResources.begin();
                      iterTimeSlot != subChannel.temporalResources.end(); ++iterTimeSlot)
                {
                    SchedulingTimeSlotPtr timeSlotPtr = *iterTimeSlot;
                    int timeSlotIndex = timeSlotPtr->timeSlotIndex;
                    //MESSAGE_SINGLE(NORMAL, logger, "doStartSubScheduling(): trying subchannel.timeslot="<<subChannelIndex<<"."<<timeSlotIndex
                    //               <<": "<<(timeSlotPtr->isEmpty()?"empty":"reserved")
                    //               <<": #="<<timeSlotPtr->countScheduledCompounds());
                    // if (timeSlotPtr->isEmpty()) // not the right question in UL slave
                    if (timeSlotPtr->countScheduledCompounds()==0)
                    { // free space found. Pack it into.
                        MESSAGE_BEGIN(NORMAL, logger, m, "Retransmitting");
                        m << " HARQ block ("<<resourceBlock->getUserID()->getName()<<",processID=" << resourceBlock->harq.processID;
                        m << ",Retry="<<resourceBlock->harq.retryCounter<<")";
                        m << " inside subchannel.timeslot=" <<subChannelIndex<<"."<<timeSlotIndex;
                        m << " (ex "<<resourceBlock->subChannelIndex<<"."<<resourceBlock->timeSlotIndex<<")";
                        MESSAGE_END();
                        assure(resourceBlock->subChannelIndex==resourceBlock->physicalResources[0].subChannelIndex,
                               "mismatch of subChannelIndex: "<<resourceBlock->subChannelIndex<<"!="<<resourceBlock->physicalResources[0].subChannelIndex);
                        assure(resourceBlock->timeSlotIndex==resourceBlock->physicalResources[0].timeSlotIndex,
                               "mismatch of timeSlotIndex: "<<resourceBlock->timeSlotIndex<<"!="<<resourceBlock->physicalResources[0].timeSlotIndex);
                        assure (resourceBlock != wns::scheduler::SchedulingTimeSlotPtr(),"resourceBlock==NULL");
                        iterSubChannel->temporalResources[timeSlotIndex] = resourceBlock; // copy Smartptr over
                        // at this point timeSlotPtr is no longer valid for use! Only resourceBlock
                        resourceBlock->subChannelIndex = subChannelIndex;
                        resourceBlock->timeSlotIndex = timeSlotIndex;
                        assure(resourceBlock->physicalResources.size()==resourceBlock->numSpatialLayers,
                               "mismatch in spatial domain: "<<resourceBlock->physicalResources.size()<<"!="<<resourceBlock->numSpatialLayers);
                        // foreach PRB... fix subChannelIndex
                        for ( PhysicalResourceBlockVector::iterator iterPRB = resourceBlock->physicalResources.begin();
                              iterPRB != resourceBlock->physicalResources.end(); ++iterPRB)
                        {
                            int spatialIndex = iterPRB->spatialIndex;
                            MESSAGE_SINGLE(NORMAL, logger, subChannelIndex<<"."<<timeSlotIndex<<".PRB["<<spatialIndex<<"]: Adjusting subChannelIndex from "<<iterPRB->subChannelIndex<<" to "<<subChannelIndex);
                            iterPRB->subChannelIndex = subChannelIndex;
                            iterPRB->timeSlotIndex = timeSlotIndex;
                        }
                        // end the loops:
                        foundSpace=true;
                        break; // found a space, so end the loops
                    } // if free space found
                    if (foundSpace) break;
                } // forall timeSlots
                if (foundSpace) break;
            }// forall subChannels
            resourceBlock = colleagues.harq->nextRetransmission();
        } // while
    } // if

    MapInfoCollectionPtr mapInfoCollection = MapInfoCollectionPtr(new wns::scheduler::MapInfoCollection);

    return mapInfoCollection;
} // doStartSubScheduling

