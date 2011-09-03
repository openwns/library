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

#include "boost/tuple/tuple.hpp"

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
    neverUsed_(true),
    numberOfFrames_(config.get<unsigned int>("numberOfFrames")),
    currentFrame_(0),
    stateTracker_(numberOfFrames_, logger),
    resources_(NULL),
    resourceGridConfig_(config.get("resourceGrid")),
    frameOccupationFairness_("scheduler.persistentvoip.FrameOccupationFairness")
{
}

PersistentVoIP::~PersistentVoIP()
{
    if(resources_ != NULL)
        delete resources_;
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

    /* Unused scheduler ex. because no data present for QoS class */
    if (neverUsed_ && activeConnections.empty()) 
        return mapInfoCollection; 

    neverUsed_ = false;

    // Init the resources but only if there ever is any data queued for this priority
    if(firstScheduling_)
        onFirstScheduling(schedulerState, schedulingMap);

    assure(resources_ != NULL, "Invalid resource grid");

    persistentvoip::StateTracker::ClassifiedConnections cc;
    cc = stateTracker_.updateState(activeConnections, currentFrame_);

    /* Take care of CIDs not active anymore */
    processSilenced(cc.silencedCIDs);

    /* Done once per period, mostly probing */
    if(currentFrame_ == 0)
        onNewPeriod();

    // nothing to do?
    if (activeConnections.empty()) 
    {
        MESSAGE_SINGLE(VERBOSE, logger, "No connection has queued data");
        return mapInfoCollection; 
    }

    resources_->onNewFrame(currentFrame_, schedulingMap);

    ConnectionSet persistentCIDs;
    persistentCIDs = cc.persistentCIDs;

    // Check TB sizes for CIDs, remove persistent CIDs needing more RBs now and return them
    ConnectionSet needMore;
    unsigned int oldSize = persistentCIDs.size();    
    needMore = updateTBSizes(persistentCIDs, schedulerState, schedulingMap);
    MESSAGE_SINGLE(NORMAL, logger, needMore.size() << " out of " << oldSize 
        << " persistent connections need more RBs than last frame.");
    // Clear the persistent reservations that do not fit anymore
    resources_->unscheduleCID(currentFrame_, needMore);

    // Calculate TB size for reactivated CIDs
    ConnectionSet test;    
    test = updateTBSizes(cc.reactivatedPersistentCIDs, schedulerState, schedulingMap);    
    assure(test.size() == 0, "Previously unpersistent CID cannot change size");

    // Try to find resources for reactivated CIDs
    ConnectionSetPair schedReactResult;
    schedReactResult = schedulePersistently(cc.reactivatedPersistentCIDs);

    // Calculate TB size for new persistent CIDs
    test = updateTBSizes(cc.newPersistentCIDs, schedulerState, schedulingMap);    
    assure(test.size() == 0, "Previously unknown CID cannot change size");

    // Try to find resources for new persistent CIDs
    ConnectionSetPair schedNewResult;
    schedNewResult = schedulePersistently(cc.newPersistentCIDs);

    // Try to find new RBs for CIDs with increased demand
    test = updateTBSizes(needMore, schedulerState, schedulingMap);
    assure(test.size() == 0, "Previously unknown CID cannot change size");
    // Try to find resources for new persistent CIDs
    ConnectionSetPair reschedResult;
    reschedResult = schedulePersistently(needMore);

    // Insert new and reactivated that got resources into persistent CID set
    // Those consume PDCCH resources to announce new persistent reservations
    persistentCIDs.insert(schedReactResult.first.begin(), schedReactResult.first.end());
    persistentCIDs.insert(schedNewResult.first.begin(), schedNewResult.first.end());
    // Those consume PDCCH resources to inform about reallocation
    persistentCIDs.insert(reschedResult.first.begin(), reschedResult.first.end());

    MESSAGE_SINGLE(NORMAL, logger, "Persistently scheduling " << persistentCIDs.size()
        << " CIDs in frame " << currentFrame_ << ".");

    MapInfoCollectionPtr result;

    ConnectionSet::iterator it;

    for(it = persistentCIDs.begin();
        it != persistentCIDs.end();
        it++)
    {

        MESSAGE_SINGLE(NORMAL, logger, "Scheduling data for CID " << *it);

        result = scheduleData(*it, true, schedulerState, schedulingMap);
        mapInfoCollection->join(*result);
    }

    MESSAGE_SINGLE(NORMAL, logger, "Dynamically scheduling " << cc.unpersistentCIDs.size()
        << " CIDs in frame " << currentFrame_ << ".");
        
    /* Now take care of unpersistent CIDs */
    for(it = cc.unpersistentCIDs.begin();
        it != cc.unpersistentCIDs.end();
        it++)
    {
        MESSAGE_SINGLE(NORMAL, logger, "Trying to schedule data for CID " << *it);

        result = scheduleData(*it, false, schedulerState, schedulingMap);
        if(result != MapInfoCollectionPtr())
            mapInfoCollection->join(*result);
    }             

    /* 
    TODO: Schedule persistent CIDs activated in this frame that 
        do not fit in this frame.
    TODO: Schedule HARQ retransmissions
    */

    return mapInfoCollection;
} // doStartSubScheduling

MapInfoCollectionPtr
PersistentVoIP::scheduleData(ConnectionID cid, bool persistent,
                             const SchedulerStatePtr& schedulerState,
                             const SchedulingMapPtr& schedulingMap)
{
    MapInfoCollectionPtr mapInfoCollection = 
        MapInfoCollectionPtr(new wns::scheduler::MapInfoCollection);;

    apcstrategy::APCResult apcResult;

    persistentvoip::TransmissionBlockPtr tb;
    if(persistent)
    {
        tb = resources_->getReservation(currentFrame_, cid, true);
        apcResult = getAPCResult(cid, schedulerState, schedulingMap);
    }
    else
    {    
        apcResult = getAPCResult(cid, schedulerState, schedulingMap);
        unsigned int nrb = getNumberOfRBs(apcResult, cid, schedulingMap);        
        bool success = resources_->scheduleCID(currentFrame_, cid, nrb, false);
        if(!success)
        {
            MESSAGE_SINGLE(NORMAL, logger, "No free resources for CID " << cid
                << " in frame " << currentFrame_ << ".");
            return mapInfoCollection;
        }
        else
        {
            tb = resources_->getReservation(currentFrame_, cid, false);
        }
    }

    unsigned int start = tb->getStart();
    unsigned int length = tb->getLength();

    MESSAGE_SINGLE(NORMAL, logger, "Scheduling data from CID " << cid << " in RBs "
        << start << " to " << start + length - 1 << ".");

    UserID user = colleagues.registry->getUserForCID(cid);

    assure(colleagues.queue->queueHasPDUs(cid), "Cannot schedule CID " << cid
            << " because it has no queued PDUs");

    /* Could be we do not need all RBs because our MCS got better, so also end
    the loop if queue is empty. This should only happen for persistent reservations */
    int i;
    Bit totalBit = 0;
    for(i = 0; i < length && colleagues.queue->queueHasPDUs(cid); i++)
    {   
        MapInfoEntryPtr mapInfoEntry; 
        mapInfoEntry = MapInfoEntryPtr(new MapInfoEntry());
        mapInfoEntry->frameNr = schedulerState->currentState->strategyInput->getFrameNr();
        mapInfoEntry->subBand = start + i;
        mapInfoEntry->timeSlot = 0;
        mapInfoEntry->spatialLayer = 0;
        mapInfoEntry->user = user;
        mapInfoEntry->sourceUser = schedulerState->myUserID;
        mapInfoEntry->txPower = apcResult.txPower; 
        mapInfoEntry->phyModePtr = apcResult.phyModePtr;
        mapInfoEntry->estimatedCQI = apcResult.estimatedCandI;

        int freeBits = schedulingMap->getFreeBitsOnSubChannel(mapInfoEntry);
        
        int queuedBits = colleagues.queue->getHeadOfLinePDUbits(cid);
        wns::ldk::CompoundPtr compoundPtr = colleagues.queue->getHeadOfLinePDUSegment(cid, freeBits);

        RequestForResource request(cid, user, freeBits, queuedBits, useHARQ);
        schedulingMap->addCompound(request, mapInfoEntry, compoundPtr, useHARQ);
        mapInfoEntry->compounds.push_back(compoundPtr);
        mapInfoCollection->push_back(mapInfoEntry);
        Bit currentBit = (queuedBits < freeBits?queuedBits:freeBits);        
        totalBit += currentBit;

        MESSAGE_SINGLE(VERBOSE, logger, "Scheduled " 
            << currentBit << " bit from CID " << cid << " in RB " << start + i);
    }
    assure(persistent || i == length, "All RBs must be used for dynamic scheduling.");

    MESSAGE_SINGLE(NORMAL, logger, "Scheduled " 
        << totalBit << " bit from CID " << cid << " in " << i << " RBs.");

    return mapInfoCollection;
}


ConnectionSet
PersistentVoIP::updateTBSizes(ConnectionSet& cids, 
                              const SchedulerStatePtr& schedulerState,
                              const SchedulingMapPtr& schedulingMap)
{
    ConnectionSet needMore;

    ConnectionSet::iterator it;
    for(it = cids.begin();
        it != cids.end();
        it++)
    {
        apcstrategy::APCResult apc;
        apc = getAPCResult(*it, schedulerState, schedulingMap);
        unsigned int rbs = getNumberOfRBs(apc, *it, schedulingMap);
        if(tbSizes_.find(*it) == tbSizes_.end())
        {
            tbSizes_[*it] = rbs;
        }
        else
        {
            MESSAGE_SINGLE(NORMAL, logger, "CID " << *it << " needed " << tbSizes_[*it]
                << " RBs bofore, now needs " << rbs << " RBs.");

            if(rbs > tbSizes_[*it])
            {
                needMore.insert(*it);
                tbSizes_.erase(*it);
            } 
        }
    }

    for(it = needMore.begin();
        it != needMore.end();
        it++)
    {
        assure(cids.find(*it) != cids.end(), "Cannot remove unknown CID " << *it);
        cids.erase(*it);
    }

    return needMore;
}

PersistentVoIP::ConnectionSetPair
PersistentVoIP::schedulePersistently(const ConnectionSet& cids)
{
    ConnectionSetPair result;

    ConnectionSet::iterator it;
    for(it = cids.begin();
        it != cids.end();
        it++)
    {
        assure(tbSizes_.find(*it) != tbSizes_.end(), "Unknown TB size for CID " << *it);
        unsigned int tbSize = tbSizes_[*it];

        bool success;
        success = resources_->scheduleCID(currentFrame_, *it, tbSize, true);
        if(success)
        {
            MESSAGE_SINGLE(NORMAL, logger, "Succesfully scheduled new CID " << *it);
            result.first.insert(*it);
        }
        else
        {
            MESSAGE_SINGLE(NORMAL, logger, "No free resources for CID " << *it);
            stateTracker_.silenceCID(*it, currentFrame_);
            tbSizes_.erase(*it);
            result.second.insert(*it);
        }
    }
    return result;
}

wns::scheduler::strategy::apcstrategy::APCResult
PersistentVoIP::getAPCResult(ConnectionID cid, 
                       const SchedulerStatePtr& schedulerState,
                       const SchedulingMapPtr& schedulingMap)
{
    UserID user = colleagues.registry->getUserForCID(cid);

    /* 
    Determine PhyMode and TxPower depending on estimation from RegistryProxy
    TODO: Interference cache is currently extremely unefficient
    should ask for quality on subchannel, not per user.
    */
    ChannelQualityOnOneSubChannel cqi;
    if(schedulerState->isTx)
        cqi = colleagues.registry->estimateTxSINRAt(user);
    else
        cqi = colleagues.registry->estimateRxSINROf(user);

    /* RequestedBits and QueuedBits is initially set to 1, will be decided later */
    RequestForResource request(cid, user, 1, 1, useHARQ);
    request.cqiOnSubChannel = cqi;

    return colleagues.strategy->getAPCStrategy()
        ->doStartAPC(request, schedulerState, schedulingMap);
}

unsigned int
PersistentVoIP::getNumberOfRBs(const apcstrategy::APCResult& apcResult, 
                               ConnectionID cid,
                               const SchedulingMapPtr& schedulingMap)
{
    Bit bitPerRB;
    bitPerRB = apcResult.phyModePtr->getBitCapacityFractional(schedulingMap->getSlotLength());
    int queuedHeadBits = colleagues.queue->getHeadOfLinePDUbits(cid);
    int requiredRBs = ceil(double(queuedHeadBits) / double(bitPerRB));    

    MESSAGE_BEGIN(NORMAL, logger, m, "RBs for " << queuedHeadBits << " per MCS: ");
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

    return requiredRBs;
}

void
PersistentVoIP::processSilenced(const ConnectionSet& cids)
{
    resources_->unscheduleCID(currentFrame_, cids);

    ConnectionSet::iterator it;

    for(it = cids.begin(); it != cids.end(); it++)
    {
        assure(tbSizes_.find(*it) != tbSizes_.end(),
            "No entry for " << *it << " in TB size map");

        tbSizes_.erase(*it);
    }
}

void
PersistentVoIP::onFirstScheduling(const SchedulerStatePtr& schedulerState,
                             wns::scheduler::SchedulingMapPtr schedulingMap)
{
    assure(firstScheduling_, "This method may only be called once.");
    firstScheduling_ = false;

    numberOfSubchannels_ = schedulerState->getCurrentState()
        ->strategyInput->getFChannels();

    MESSAGE_SINGLE(NORMAL, logger, "Creating resource grid with " << numberOfSubchannels_ 
        << " resources per frame in " << numberOfFrames_ << " frames.");

    resources_ = new persistentvoip::ResourceGrid(resourceGridConfig_, logger, 
        numberOfFrames_, numberOfSubchannels_, colleagues.registry, schedulingMap->getSlotLength());
}

void
PersistentVoIP::onNewPeriod()
{
#ifndef WNS_NDEBUG
    for(int i = 0; i < numberOfFrames_; i++)
        MESSAGE_SINGLE(NORMAL, logger, *resources_->getFrame(i));    
#endif

    /* 
    Calculate Jain's fairness index J:
    J = 1: All frames have same amount of reserved RBs
    J -> 0: The lower J the more does the number of reserved RBs differ 
    */
    if(
#ifndef WNS_NDEBUG
        true)
#else
        frameOccupationFairness_.hasObservers())
#endif
    {
        double sum;
        double squareSum;
        for(int i = 0; i < numberOfFrames_; i++)
        {
            unsigned int nr = resources_->getFrame(i)->getNumReserved();
            sum += nr;
            squareSum += (nr * nr);
        }
        if(squareSum > 0)
        {
            double fairness = (sum * sum) / (double(numberOfFrames_) * squareSum);

            unsigned int nodeID = colleagues.registry->getMyUserID().getNodeID();

            frameOccupationFairness_.put(fairness, boost::make_tuple("nodeID",nodeID));

            MESSAGE_SINGLE(NORMAL, logger, "Fairness index: " << fairness);
            assure(fairness >= 0 && fairness <= 1, "Fairness index out of range");
        }
    }
}
