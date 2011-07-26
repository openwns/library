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

#include <WNS/scheduler/strategy/staticpriority/PersistentVoIP.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>
#include <WNS/scheduler/strategy/apcstrategy/APCStrategyInterface.hpp>

#include <vector>
#include <map>
#include <algorithm>
#include <iostream>

using namespace std;
using namespace wns::scheduler;
using namespace wns::scheduler::strategy;
using namespace wns::scheduler::strategy::staticpriority;

STATIC_FACTORY_REGISTER_WITH_CREATOR(PersistentVoIP,
                                     SubStrategyInterface,
                                     "PersistentVoIP",
                                     wns::PyConfigViewCreator);


PersistentVoIP::PersistentVoIP(const wns::pyconfig::View& config)
    : SubStrategy(config),
    numberOfFrames_(config.get<unsigned int>("numberOfFrames")),
    currentFrame_(0),
    persistentSchedule_(numberOfFrames_)
{
}

PersistentVoIP::~PersistentVoIP()
{
}

void
PersistentVoIP::initialize()
{
}


wns::scheduler::MapInfoCollectionPtr
PersistentVoIP::doStartSubScheduling(SchedulerStatePtr schedulerState,
                                 wns::scheduler::SchedulingMapPtr schedulingMap)
{
    currentFrame_++;
    currentFrame_ %= numberOfFrames_;

    MapInfoCollectionPtr mapInfoCollection = 
        MapInfoCollectionPtr(new wns::scheduler::MapInfoCollection); // result datastructure

    ConnectionSet currentConnections = schedulerState->currentState->activeConnections;

    ConnectionSet activeConnections = colleagues.queue->filterQueuedCids(currentConnections);

    // nothing to do?
    if (activeConnections.empty()) 
    {
        MESSAGE_SINGLE(VERBOSE, logger, "No connection has queued data");
        return mapInfoCollection; 
    }

    updateState(activeConnections);

    MESSAGE_SINGLE(NORMAL, logger, "Now scheduling frame " << currentFrame_ << " with " 
        << activeConnections.size() << " active connections.");

    ConnectionSet::iterator it;

    /* For every connection that has queued PDUs*/
    for(it = activeConnections.begin();
        it != activeConnections.end();
        it++)
    {
        ConnectionID cid = *it;
        UserID user = colleagues.registry->getUserForCID(cid);

        persistentSchedule_[currentFrame_].insert(cid);

        MESSAGE_SINGLE(NORMAL, logger, "Now scheduling CID " << cid << " User: " << user.getName());

        /* Determine PhyMode and TxPower depending on estimation from RegistryProxy */
        ChannelQualityOnOneSubChannel cqi;
        if(schedulerState->isTx)
            cqi = colleagues.registry->estimateTxSINRAt(user);
        else
            cqi = colleagues.registry->estimateRxSINROf(user);

        /* RequestedBits and QueuedBits is initially set to 1, will be decided later */
        RequestForResource request(cid, user, 1, 1, useHARQ);
        request.cqiOnSubChannel = cqi;
        apcstrategy::APCResult apcResult;
        apcResult = colleagues.strategy->getAPCStrategy()
            ->doStartAPC(request, schedulerState, schedulingMap);

        Bit bitPerRB;
        bitPerRB = apcResult.phyModePtr->getBitCapacityFractional(schedulingMap->getSlotLength());
        int queuedHeadBits = colleagues.queue->getHeadOfLinePDUbits(cid);
        int requiredRBs = ceil(double(queuedHeadBits) / double(bitPerRB));

        MESSAGE_BEGIN(NORMAL, logger, m, "RBs for PhyMode: ");
        int cnt = colleagues.registry->getPhyModeMapper()->getPhyModeCount();
        for(int i = 0; i < cnt; i++)
        {
            Bit cap = colleagues.registry->getPhyModeMapper()->getPhyModeForIndex(i)
                ->getBitCapacityFractional(schedulingMap->getSlotLength());
            int chosen = colleagues.registry
                ->getPhyModeMapper()->getIndexForPhyMode(*apcResult.phyModePtr);
            if(chosen == i)
                m << ">";
            m << ceil(double(queuedHeadBits) / double(cap));
            if(chosen == i)
                m << "< ";
            else
                m << " ";
        }
        MESSAGE_END()

        MESSAGE_SINGLE(NORMAL, logger, "Estimated SINR " << apcResult.sinr
            << " for user " << user.getName()
            << " fits "
            << bitPerRB 
            << " bit per RB.");

        MESSAGE_SINGLE(NORMAL, logger, "Need " << requiredRBs 
            << " RBs to transmit " << queuedHeadBits << " bit.");

        /* Schedule every PDU */
        int npdu = 0;
        Bit schedBit = 0;
        do
        {
            int freeBits = 0;
            MapInfoEntryPtr mapInfoEntry; 
            bool free = false;

            MESSAGE_SINGLE(NORMAL, logger, "Trying to schedule PDU number " << npdu + 1 << " from CID " << cid);

            /* Try to find a subchannel */
            int sc = 0;
            do
            {
                free = false;
                mapInfoEntry = MapInfoEntryPtr(new MapInfoEntry());
                mapInfoEntry->frameNr = schedulerState->currentState->strategyInput->getFrameNr();
                mapInfoEntry->subBand = sc;
                mapInfoEntry->timeSlot = 0;
                mapInfoEntry->spatialLayer = 0;
                mapInfoEntry->user = user;
                mapInfoEntry->sourceUser = schedulerState->myUserID;
                mapInfoEntry->txPower = apcResult.txPower; 
                mapInfoEntry->phyModePtr = apcResult.phyModePtr;
                mapInfoEntry->estimatedCQI = cqi;

                UserID prbUID = schedulingMap->subChannels[sc].temporalResources[0]->physicalResources[0].getUserID();

                if(prbUID == mapInfoEntry->user || !prbUID.isValid())
                {
                    freeBits = schedulingMap->getFreeBitsOnSubChannel(mapInfoEntry);
                    if(freeBits > 0)
                    {
                        free = true;
                    }
                    else
                    {
                        free = false;
                    }
                }
                else
                {
                    free = false;
                }

                MESSAGE_SINGLE(NORMAL, logger, "Probing SubChannel " << sc
                    << ": User: " << (prbUID.isBroadcast()?"Broadcast":
                        (prbUID.isValid()?prbUID.getName():"None"))
                    << ", free bit: " << freeBits
                    << ", usable: " << (free?"Yes":"No"));

                sc++;
            }
            while(sc < schedulingMap->getNumberOfSubChannels() && !free);

            /* No free subchannels left, exit */
            if(sc == schedulingMap->getNumberOfSubChannels())
            {
                MESSAGE_SINGLE(NORMAL, logger, "Scheduling stopped. No more free subchannels");
                return mapInfoCollection;
            }
                
            /* Insert the information about the scheduled compound into the RB */
            int queuedBits = colleagues.queue->getHeadOfLinePDUbits(cid);
            wns::ldk::CompoundPtr compoundPtr = colleagues.queue->getHeadOfLinePDUSegment(cid, freeBits);
            request.bits = freeBits;
            request.queuedBits = queuedBits;            
            schedulingMap->addCompound(request, mapInfoEntry, compoundPtr, useHARQ);
            mapInfoEntry->compounds.push_back(compoundPtr); 

            /* Write the scheduling decision for this RB into the map */
            mapInfoCollection->push_back(mapInfoEntry);
            MESSAGE_SINGLE(NORMAL, logger, "Scheduled " 
                << (queuedBits < freeBits?queuedBits:freeBits) << " bit of " 
                << queuedBits << " bit from CID " 
                << cid << " in subchannel " << sc - 1);


            schedBit += (queuedBits < freeBits?queuedBits:freeBits);
            npdu++;
        }
        while(colleagues.queue->queueHasPDUs(cid));
        MESSAGE_SINGLE(NORMAL, logger, "Finished scheduling CID " << cid);
        MESSAGE_SINGLE(NORMAL, logger, "RBs expected / scheduled: " << requiredRBs
            << " / " << npdu << ", bit expected / scheduled: "
            << queuedHeadBits << " / " << schedBit);
    }
    return mapInfoCollection;
} // doStartSubScheduling

void
PersistentVoIP::updateState(const ConnectionSet activeConnections)
{
    ConnectionSet::iterator it;
    MESSAGE_BEGIN(NORMAL, logger, m, "CIDs expected in frame " << currentFrame_ << ": ");
    for(it = persistentSchedule_[currentFrame_].begin();
        it != persistentSchedule_[currentFrame_].end();    
        it++)
    {
        m << *it << " ";
    }
    MESSAGE_END();    

    MESSAGE_BEGIN(NORMAL, logger, m, "CIDs active in frame " << currentFrame_ << ": ");
    for(it = activeConnections.begin();
        it != activeConnections.end();    
        it++)
    {
        m << *it << " ";
    }
    MESSAGE_END();    

    ConnectionSet inactiveConnections;
    for(it = persistentSchedule_[currentFrame_].begin();
        it != persistentSchedule_[currentFrame_].end();    
        it++)
    {
        if(activeConnections.find(*it) == activeConnections.end())
        {
            inactiveConnections.insert(*it);
            persistentSchedule_[currentFrame_].erase(*it);
        }
    }

    MESSAGE_BEGIN(NORMAL, logger, m, "CIDs no longer active in frame " << currentFrame_ << ": ");
    for(it = inactiveConnections.begin();
        it != inactiveConnections.end();    
        it++)
    {
        m << *it << " ";
    }
    MESSAGE_END();  

    ConnectionSet oldConnections;
    std::insert_iterator<ConnectionSet> ii(oldConnections, oldConnections.begin()); 
    set_intersection(persistentSchedule_[currentFrame_].begin(), 
        persistentSchedule_[currentFrame_].end(), 
        activeConnections.begin(), 
        activeConnections.end(), ii);

    MESSAGE_BEGIN(NORMAL, logger, m, "Previously known CIDs in frame " << currentFrame_ << ": ");
    ConnectionSet::iterator it;
    for(it = oldConnections.begin();
        it != oldConnections.end();    
        it++)
    {
        m << *it << " ";
    }
    MESSAGE_END();  
}



