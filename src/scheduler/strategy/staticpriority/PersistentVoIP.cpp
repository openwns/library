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
    firstScheduling_(true),
    numberOfFrames_(config.get<unsigned int>("numberOfFrames")),
    currentFrame_(0),
    expectedCIDs_(numberOfFrames_),
    pastPeriodCIDs_(numberOfFrames_)
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

    if(firstScheduling_)
        onFirstScheduling(schedulerState);

    MapInfoCollectionPtr mapInfoCollection = 
        MapInfoCollectionPtr(new wns::scheduler::MapInfoCollection); // result datastructure

    ConnectionSet currentConnections = schedulerState->currentState->activeConnections;

    ConnectionSet activeConnections = colleagues.queue->filterQueuedCids(currentConnections);

    updateState(activeConnections);

    // nothing to do?
    if (activeConnections.empty()) 
    {
        MESSAGE_SINGLE(VERBOSE, logger, "No connection has queued data");
        return mapInfoCollection; 
    }

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

                MESSAGE_SINGLE(VERBOSE, logger, "Probing SubChannel " << sc
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
PersistentVoIP::updateState(const ConnectionSet activeCIDs)
{
    if(activeCIDs.size() == 0 && 
        expectedCIDs_[currentFrame_].size() == 0)
    {
        pastPeriodCIDs_[currentFrame_].clear();
        return;
    }

    ConnectionSet::iterator it;

    MESSAGE_SINGLE(NORMAL, logger, "------------------------ Update CIDs frame: " 
        << currentFrame_ << " -------------------" );

    MESSAGE_BEGIN(NORMAL, logger, m, "CIDs active: ");
    for(it = activeCIDs.begin();
        it != activeCIDs.end();    
        it++)
    {
        m << *it << " ";
    }
    MESSAGE_END();    

    MESSAGE_BEGIN(NORMAL, logger, m, "CIDs expected: ");
    for(it = expectedCIDs_[currentFrame_].begin();
        it != expectedCIDs_[currentFrame_].end();    
        it++)
    {
        m << *it << " ";
    }
    MESSAGE_END();    

    // Currently present and present in this frame before
    ConnectionSet oldCIDs;
    std::insert_iterator<ConnectionSet> iiOld(oldCIDs, oldCIDs.begin()); 
    set_intersection(expectedCIDs_[currentFrame_].begin(), 
        expectedCIDs_[currentFrame_].end(), 
        activeCIDs.begin(), 
        activeCIDs.end(), iiOld);

    MESSAGE_BEGIN(NORMAL, logger, m, "CIDs expected and active: ");
    for(it = oldCIDs.begin();
        it != oldCIDs.end();    
        it++)
    {
        m << *it << " ";
    }
    MESSAGE_END();  

    // Expected but not there => silenced 
    ConnectionSet silencedCIDs;
    std::insert_iterator<ConnectionSet> iiSilenced(silencedCIDs, silencedCIDs.begin()); 
    set_difference(expectedCIDs_[currentFrame_].begin(), 
        expectedCIDs_[currentFrame_].end(), 
        activeCIDs.begin(), 
        activeCIDs.end(), iiSilenced);

    MESSAGE_BEGIN(NORMAL, logger, m, "CIDs not active anymore (silenced): ");
    for(it = silencedCIDs.begin();
        it != silencedCIDs.end();    
        it++)
    {
        m << *it << " ";
    }
    MESSAGE_END();

    // Currently present but not known yet
    ConnectionSet unknownCIDs;
    std::insert_iterator<ConnectionSet> iiUnknown(unknownCIDs, unknownCIDs.begin()); 
    set_difference(activeCIDs.begin(), 
        activeCIDs.end(), 
        allCIDs_.begin(), 
        allCIDs_.end(), iiUnknown);

    // We saw those last period but never before => new CIDs
    ConnectionSet newCIDs;
    std::insert_iterator<ConnectionSet> iiNew(newCIDs, newCIDs.begin()); 
    set_intersection(unknownCIDs.begin(), 
        unknownCIDs.end(), 
        pastPeriodCIDs_[currentFrame_].begin(), 
        pastPeriodCIDs_[currentFrame_].end(), iiNew);

    MESSAGE_BEGIN(NORMAL, logger, m, "New CIDs: ");
    for(it = newCIDs.begin();
        it != newCIDs.end();    
        it++)
    {
        m << *it << " ";
    }
    MESSAGE_END();  

    // This CID was not there last period, see if it will be there next one
    ConnectionSet firstTimeCIDs;
    std::insert_iterator<ConnectionSet> iiFT(firstTimeCIDs, firstTimeCIDs.begin()); 
    set_difference(unknownCIDs.begin(), 
        unknownCIDs.end(), 
        newCIDs.begin(), 
        newCIDs.end(), iiFT);

    // Currently present and known
    ConnectionSet knownCIDs;
    std::insert_iterator<ConnectionSet> iiKnown(knownCIDs, knownCIDs.begin()); 
    set_difference(activeCIDs.begin(), 
        activeCIDs.end(), 
        unknownCIDs.begin(), 
        unknownCIDs.end(), iiKnown);

    // Currently present and known but not expected
    ConnectionSet unexpectedCIDs;
    std::insert_iterator<ConnectionSet> iiUnexp(unexpectedCIDs, unexpectedCIDs.begin()); 
    set_difference(knownCIDs.begin(),
        knownCIDs.end(),
        expectedCIDs_[currentFrame_].begin(), 
        expectedCIDs_[currentFrame_].end(), iiUnexp);

    // Currently present and known but not expected 
    // but seen in last period => talking again
    ConnectionSet reactivatedCIDs;
    std::insert_iterator<ConnectionSet> iiReact(reactivatedCIDs, reactivatedCIDs.begin()); 
    set_intersection(unexpectedCIDs.begin(),
        unexpectedCIDs.end(),
        pastPeriodCIDs_[currentFrame_].begin(), 
        pastPeriodCIDs_[currentFrame_].end(), iiReact);

    MESSAGE_BEGIN(NORMAL, logger, m, "Reactivated CIDs: ");
    for(it = reactivatedCIDs.begin();
        it != reactivatedCIDs.end();    
        it++)
    {
        m << *it << " ";
    }
    MESSAGE_END();
    for(it = reactivatedCIDs.begin();
        it != reactivatedCIDs.end();    
        it++)
    {
        assure(silentCIDs_.find(*it) != silentCIDs_.end(),
            "Reactivated CID " << *it << " was not silenced before.");
        silentCIDs_.erase(*it);
    }

    // Currently present, silenced and known but not expected 
    // and not seen in last period => comfort noise or talking again
    //
    // Unexpected CIDs that are not in silent mode must have
    // leftover PDUs that did not fit before
    ConnectionSet possibleReactCIDs;
    std::insert_iterator<ConnectionSet> iiPosReact(possibleReactCIDs, possibleReactCIDs.begin()); 
    set_difference(unexpectedCIDs.begin(),
        unexpectedCIDs.end(),
        pastPeriodCIDs_[currentFrame_].begin(), 
        pastPeriodCIDs_[currentFrame_].end(), iiPosReact);
    ConnectionSet possibleReactSilCIDs;
    std::insert_iterator<ConnectionSet> iiPosReactSil(possibleReactSilCIDs, possibleReactSilCIDs.begin()); 
    set_intersection(possibleReactCIDs.begin(),
        possibleReactCIDs.end(),
        silentCIDs_.begin(), 
        silentCIDs_.end(), iiPosReactSil);

    MESSAGE_BEGIN(NORMAL, logger, m, "Comfort noise CIDs: ");
    for(it = possibleReactSilCIDs.begin();
        it != possibleReactSilCIDs.end();    
        it++)
    {
        m << *it << " ";
    }
    MESSAGE_END();

    // Those are known VoIP connections
    allCIDs_.insert(newCIDs.begin(), newCIDs.end());

    // Check next period if these CIDs are still present
    pastPeriodCIDs_[currentFrame_] = firstTimeCIDs;
    pastPeriodCIDs_[currentFrame_].insert(possibleReactSilCIDs.begin(), possibleReactSilCIDs.end());

    // We expect these CIDs next period
    expectedCIDs_[currentFrame_] = oldCIDs;
    expectedCIDs_[currentFrame_].insert(newCIDs.begin(), newCIDs.end());
    expectedCIDs_[currentFrame_].insert(reactivatedCIDs.begin(), reactivatedCIDs.end());

    // Those CIDs are now inactive
    silentCIDs_.insert(silencedCIDs.begin(), silencedCIDs.end());

    MESSAGE_SINGLE(NORMAL, logger, "------------------------ Update CIDs done-------------------------" );
}

void
PersistentVoIP::onFirstScheduling(const SchedulerStatePtr& schedulerState)
{
    assure(firstScheduling_, "This method may only be called once.");
    firstScheduling_ = false;

    numberOfSubchannels_ = schedulerState->getCurrentState()
        ->strategyInput->getFChannels();

    transmissionBlocks_.resize(numberOfFrames_);

    for(int i = 0; i < numberOfFrames_; i++)
        transmissionBlocks_[i].insert(persistentvoip::TransmissionBlock(0, numberOfSubchannels_));

    MESSAGE_SINGLE(NORMAL, logger, "Managing " << numberOfSubchannels_ 
        << " resources per frame in " << numberOfFrames_ << " frames.");
}

