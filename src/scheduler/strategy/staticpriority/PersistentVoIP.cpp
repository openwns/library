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
    voicePDUSize_(config.get<Bit>("voicePDUSize")),
    resourceGridConfig_(config.get("resourceGrid")),
    frameOccupationFairness_("scheduler.persistentvoip.FrameOccupationFairness"),
    activeCIDs_("scheduler.persistentvoip.ActiveConnections"),
    allActiveCIDs_("scheduler.persistentvoip.AllActiveConnections"),
    queuedCIDs_("scheduler.persistentvoip.QueuedConnections"),
    timeRelocatedCIDs_("scheduler.persistentvoip.TimeRelocatedConnections"),
    freqRelocatedCIDs_("scheduler.persistentvoip.FreqRelocatedConnections"),
    numPDCCH_("scheduler.persistentvoip.NumberOfPDCCHs"),
    numPersRelocPDCCH_("scheduler.persistentvoip.NumberOfPersRelocationPDCCHs"),
    numPersSetupPDCCH_("scheduler.persistentvoip.NumberOfPersSetupPDCCHs"),
    numDynamicPDCCH_("scheduler.persistentvoip.NumberOfDynamicPDCCHs"),
    numSID_PDCCH_("scheduler.persistentvoip.NumberOfSID_PDCCHs"),
    numOtherFrame_PDCCH_("scheduler.persistentvoip.NumberOfOtherFramePDCCHs"),
    numHARQ_PDCCH_("scheduler.persistentvoip.NumberOfHARQ_PDCCHs"),
    numHARQ_Users_("scheduler.persistentvoip.NumberOfHARQ_Users")
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
    unsigned int nodeID = colleagues.registry->getMyUserID().getNodeID();

    activeCIDs_.put(cc.totalAppActive, boost::make_tuple("nodeID",nodeID));

    allActiveCIDs_.put(cc.totalActive, boost::make_tuple("nodeID",nodeID));

    queuedCIDs_.put(cc.totalQueued, boost::make_tuple("nodeID",nodeID));

    resources_->onNewFrame(currentFrame_, schedulingMap);

    ConnectionSet persistentCIDs;
    persistentCIDs = cc.persistentCIDs;
    ConnectionSet relocatedCIDs;

    // Check TB sizes for CIDs, remove persistent CIDs needing more RBs now and return them
    ConnectionSet needMore;
    unsigned int oldSize = persistentCIDs.size();    

    needMore = checkTBSizes(persistentCIDs);
    MESSAGE_SINGLE(NORMAL, logger, needMore.size() << " out of " << oldSize 
        << " persistent connections need more RBs than last frame.");
    // Clear the persistent reservations that do not fit anymore
    resources_->unscheduleCID(currentFrame_, needMore);
    freqRelocatedCIDs_.put(needMore.size(), boost::make_tuple("nodeID",nodeID));

    // Try to find resources for reactivated CIDs
    ConnectionSetPair schedReactResult;
    schedReactResult = schedulePersistently(cc.reactivatedPersistentCIDs);
    if(schedReactResult.second.size() > 0)
        relocatedCIDs = schedReactResult.second;
    // TODO: Probe success and failure

    // Try to find resources for new persistent CIDs
    ConnectionSetPair schedNewResult;
    schedNewResult = schedulePersistently(cc.newPersistentCIDs);
    if(schedNewResult.second.size() > 0)
        relocatedCIDs.insert(schedNewResult.second.begin(), schedNewResult.second.end());
    // TODO: Probe success and failure

    // Try to find resources for persistent CIDs that do not fit their previous reservation
    ConnectionSetPair reschedResult;
    reschedResult = schedulePersistently(needMore);
    if(reschedResult.second.size() > 0)
        relocatedCIDs.insert(reschedResult.second.begin(), reschedResult.second.end());
    // TODO: Probe success and failure

    // Insert new and reactivated that got resources into persistent CID set
    // Those consume PDCCH resources to announce new persistent reservations
    persistentCIDs.insert(schedReactResult.first.begin(), schedReactResult.first.end());
    persistentCIDs.insert(schedNewResult.first.begin(), schedNewResult.first.end());
    // Those consume PDCCH resources to inform about reallocation
    persistentCIDs.insert(reschedResult.first.begin(), reschedResult.first.end());

    // Find new frames for CIDs not fitting this frame
    if(relocatedCIDs.size() > 0)
        relocateCIDs(relocatedCIDs);

    timeRelocatedCIDs_.put(relocatedCIDs.size(), boost::make_tuple("nodeID",nodeID));

    unsigned int numPersSetupPDCCH = schedReactResult.first.size() 
        + schedReactResult.first.size();        

    unsigned int numPersRelocPDCCH = reschedResult.first.size();

    numPersSetupPDCCH_.put(numPersSetupPDCCH, boost::make_tuple("nodeID",nodeID));
    numPersRelocPDCCH_.put(numPersRelocPDCCH, boost::make_tuple("nodeID",nodeID));

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

    // TODO: Everything below here is only possible if enough PDCCH resources are available

    /* Now take care of unpersistent CIDs */
    unsigned int numSID_PDCCH = 0;

    for(it = cc.unpersistentCIDs.begin();
        it != cc.unpersistentCIDs.end();
        it++)
    {
        MESSAGE_SINGLE(NORMAL, logger, "Trying to schedule data for CID " << *it);

        /* TODO: Probe how many CIDs did not get resources */
        result = scheduleData(*it, false, schedulerState, schedulingMap);
        if(result->size() > 0)
        {
            mapInfoCollection->join(*result);
            numSID_PDCCH++;
        }
        else
        {
            stateTracker_.unservedCID(*it);
        }
    }             

    /* Give what is left to CIDs belonging to other frames */
    unsigned int numOtherFrame_PDCCH = 0; 

    for(it = cc.otherFrameCIDs.begin();
        it != cc.otherFrameCIDs.end();
        it++)
    {
        MESSAGE_SINGLE(NORMAL, logger, "Trying to schedule data for CID " << *it 
            << " from other frame");

        /* TODO: Probe how many CIDs did not get resources */
        result = scheduleData(*it, false, schedulerState, schedulingMap);
        if(result->size() > 0)
        {
            mapInfoCollection->join(*result);
            numOtherFrame_PDCCH++;
            stateTracker_.servedInOtherFrameCID(*it);
        }
        else
        {
        // TODO: Probe failure
        }
    }
    unsigned int numDynamicPDCCH = numSID_PDCCH + numOtherFrame_PDCCH;   
    numDynamicPDCCH_.put(numDynamicPDCCH, boost::make_tuple("nodeID",nodeID));
    numSID_PDCCH_.put(numSID_PDCCH, boost::make_tuple("nodeID",nodeID));
    numOtherFrame_PDCCH_.put(numOtherFrame_PDCCH, boost::make_tuple("nodeID",nodeID));

    unsigned int numHARQ_PDCCH = probeHARQ();

    unsigned int numPDCCH = numDynamicPDCCH + numPersSetupPDCCH 
        + numPersRelocPDCCH + numHARQ_PDCCH;
    numPDCCH_.put(numPDCCH, boost::make_tuple("nodeID",nodeID));

    return mapInfoCollection;
} // doStartSubScheduling

MapInfoCollectionPtr
PersistentVoIP::scheduleData(ConnectionID cid, bool persistent,
                             const SchedulerStatePtr& schedulerState,
                             const SchedulingMapPtr& schedulingMap)
{
    MapInfoCollectionPtr mapInfoCollection = 
        MapInfoCollectionPtr(new wns::scheduler::MapInfoCollection);;

    persistentvoip::TransmissionBlockPtr tb;
    if(persistent)
    {
        tb = resources_->getReservation(currentFrame_, cid, true);
    }
    else
    {    
        Bit pduSize = colleagues.queue->getHeadOfLinePDUbits(cid);

        /* 
        There could be multiple PDUs queued. getHoLPDUBits cannot
        know the HoL PDU Size in the UL, only the total queue length.
        Limit the request size to voicePDUSize.
        */
        pduSize = std::min(pduSize, voicePDUSize_);

        bool success = resources_->scheduleCID(currentFrame_, cid, pduSize, false);
        if(!success)
        {
            MESSAGE_SINGLE(NORMAL, logger, "No free resources for CID " << cid
                << " (" << colleagues.registry->getUserForCID(cid) << ")"
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
        mapInfoEntry->txPower = tb->getTxPower();
        mapInfoEntry->phyModePtr = tb->getMCS();

        /* SINR Estimation */
        mapInfoEntry->estimatedCQI.effectiveSINR = tb->getEstimatedSINR();
        mapInfoEntry->estimatedCQI.timeSlot = currentFrame_;
        if(colleagues.strategy->getSchedulerSpotType() == 
            wns::scheduler::SchedulerSpot::DLMaster())
        {
            ChannelQualityOnOneSubChannel est;
            est = colleagues.registry->estimateTxSINRAt(user, mapInfoEntry->subBand);
            mapInfoEntry->estimatedCQI.pathloss = est.pathloss;
            mapInfoEntry->estimatedCQI.interference = est.interference;
            mapInfoEntry->estimatedCQI.carrier = mapInfoEntry->txPower / est.pathloss;
        }
        else
        {
            ChannelQualityOnOneSubChannel est;
            est = colleagues.registry->estimateRxSINROf(user, mapInfoEntry->subBand);
            mapInfoEntry->estimatedCQI.pathloss = est.pathloss;
            mapInfoEntry->estimatedCQI.interference = 
                colleagues.registry->estimateTxSINRAt(schedulerState->myUserID, 
                    mapInfoEntry->subBand, currentFrame_).interference;
            mapInfoEntry->estimatedCQI.carrier = mapInfoEntry->txPower / est.pathloss;
        }

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
PersistentVoIP::checkTBSizes(ConnectionSet& cids)
{
    ConnectionSet needMore;

    ConnectionSet::iterator it;
    for(it = cids.begin();
        it != cids.end();
        it++)
    {
        assure(resources_->hasPersistentReservation(currentFrame_, *it), 
            "No persistent reservation for CID " << *it);

        Bit pduSize = colleagues.queue->getHeadOfLinePDUbits(*it);
        /* 
        There could be multiple PDUs queued. getHoLPDUBits cannot
        know the HoL PDU Size in the UL, only the total queue length.
        Limit the request size to voicePDUSize.
        */
        pduSize = std::min(pduSize, voicePDUSize_);

        bool fits = resources_->fitsPersistentReservation(currentFrame_, *it, pduSize);

        if(!fits)
        {
            needMore.insert(*it);

            MESSAGE_SINGLE(NORMAL, logger, "Persistent CID " << *it 
                << " does not fit its reservation anymore.");
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
        Bit pduSize = colleagues.queue->getHeadOfLinePDUbits(*it);

        /* 
        There could be multiple PDUs queued. getHoLPDUBits cannot
        know the HoL PDU Size in the UL, only the total queue length.
        Limit the request size to voicePDUSize.
        */
        pduSize = std::min(pduSize, voicePDUSize_);

        bool success;
        success = resources_->scheduleCID(currentFrame_, *it, pduSize, true);
        if(success)
        {
            MESSAGE_SINGLE(NORMAL, logger, "Succesfully scheduled new CID " << *it);
            result.first.insert(*it);
        }
        else
        {
            MESSAGE_SINGLE(NORMAL, logger, "No free resources for persistent CID " 
                << *it << " (" << colleagues.registry->getUserForCID(*it) << ")"
                << " in frame " << currentFrame_);
            stateTracker_.silenceCID(*it, currentFrame_);
            result.second.insert(*it);
        }
    }
    return result;
}
void
PersistentVoIP::processSilenced(const ConnectionSet& cids)
{
    resources_->unscheduleCID(currentFrame_, cids);
}

void
PersistentVoIP::relocateCIDs(const ConnectionSet& cids)
{
#ifndef WNS_NDEBUG
    std::map<ConnectionID, unsigned int> newFrames;
#endif

    ConnectionSet::iterator it;
    for(it = cids.begin(); it != cids.end(); it++)
    {
        unsigned int newFrame = resources_->getMostEmptyFrame();
        stateTracker_.relocateCID(*it, currentFrame_, newFrame);
#ifndef WNS_NDEBUG
        newFrames[*it] = newFrame;
#endif
    }
#ifndef WNS_NDEBUG
    MESSAGE_BEGIN(NORMAL, logger, m, "Relocated in frame " << currentFrame_ << ": ");
    for(it = cids.begin();
        it != cids.end();    
        it++)
    {
        m << *it 
          << " (" << colleagues.registry->getUserForCID(*it) << ")"
          << " -> " << newFrames[*it] << " ";
    }
    MESSAGE_END();
#endif
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
        numberOfFrames_, numberOfSubchannels_, 
        colleagues.registry, schedulingMap->getSlotLength(),
        colleagues.strategy->getSchedulerSpotType());
}

unsigned int
PersistentVoIP::probeHARQ()
{
    unsigned int nodeID = colleagues.registry->getMyUserID().getNodeID();
    wns::scheduler::UserSet us = colleagues.harq->getPeersWithPendingRetransmissions();
    numHARQ_Users_.put(us.size(), boost::make_tuple("nodeID",nodeID));
    wns::scheduler::UserSet::iterator user;

    unsigned numHARQ_PDCCH = 0;
    for(user = us.begin(); user!=us.end(); ++user)
    {
        std::list<int> processesToSchedule = 
            colleagues.harq->getPeerProcessesWithRetransmissions(*user);

        numHARQ_PDCCH+= processesToSchedule.size();
    }
    numHARQ_PDCCH_.put(numHARQ_PDCCH, boost::make_tuple("nodeID",nodeID));
    return numHARQ_PDCCH;
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
        double sum = 0;
        double squareSum = 0;
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
