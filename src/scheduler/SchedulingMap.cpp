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

#include <WNS/scheduler/SchedulerTypes.hpp>
#include <WNS/scheduler/SchedulingMap.hpp>
#include <WNS/scheduler/strategy/SchedulerState.hpp> // for RequestForResource
#include <WNS/ldk/Compound.hpp>
#include <WNS/SmartPtr.hpp>
#include <WNS/service/phy/phymode/PhyModeInterface.hpp>
#include <iostream>
#include <fstream>

using namespace wns::scheduler;

SchedulingCompound::SchedulingCompound()
  : subChannel(0),
    timeSlot(0),
    beam(0),
    startTime(0.0),
    endTime(0.0),
    connectionID(-1),
    userID(NULL),
    compoundPtr(),
    phyModePtr(),
    txPower()
{
};

SchedulingCompound::SchedulingCompound(int _subChannel,
                                       int _timeSlot,
                                       int _beam,
                                       simTimeType _startTime,
                                       simTimeType _endTime,
                                       wns::scheduler::ConnectionID _connectionID,
                                       wns::scheduler::UserID _userID,
                                       wns::ldk::CompoundPtr _compoundPtr,
                                       wns::service::phy::phymode::PhyModeInterfacePtr _phyModePtr,
                                       wns::Power _txPower,
                                       wns::service::phy::ofdma::PatternPtr _pattern
    )
    : subChannel(_subChannel),
      timeSlot(_timeSlot),
      beam(_beam),
      startTime(_startTime),
      endTime(_endTime),
      connectionID(_connectionID),
      userID(_userID),
      compoundPtr(_compoundPtr),
      phyModePtr(_phyModePtr),
      txPower(_txPower),
      pattern(_pattern)
{
}

SchedulingCompound::~SchedulingCompound()
{
}

std::string
SchedulingCompound::toString() const
{
    std::stringstream s;
    s.setf(std::ios::fixed,std::ios::floatfield);   // floatfield set to fixed
    s.precision(1);
    s << "SchedulingCompound(";
    s << "cid="<<connectionID;
    if (userID!=NULL) {
        s << ", user="<<userID->getName();
    }
    if (compoundPtr != wns::ldk::CompoundPtr()) {
        s << ", bits="<< compoundPtr->getLengthInBits();
    }
    s << ", T=[" << startTime*1e6 << "-" << endTime*1e6 << "]us";
    s << ", d=" << (endTime-startTime)*1e6 << "us";
    s << ")";
    return s.str();
}

/**************************************************************/
PhysicalResourceBlock::PhysicalResourceBlock()
    : subChannelIndex(0),
      timeSlotIndex(0),
      beamIndex(0),
      slotLength(0.0),
      freeTime(0.0),
      nextPosition(0.0),
      scheduledCompounds(),
      phyModePtr(),
      txPower(),
      antennaPattern()
{
}

PhysicalResourceBlock::PhysicalResourceBlock(int _subChannelIndex, int _timeSlotIndex, int _beam, simTimeType _slotLength)
    : subChannelIndex(_subChannelIndex),
      timeSlotIndex(_timeSlotIndex),
      beamIndex(_beam),
      slotLength(_slotLength),
      freeTime(_slotLength),
      nextPosition(0.0),
      scheduledCompounds(),
      userID(NULL),
      phyModePtr(),
      txPower(),
      antennaPattern()
{
}

PhysicalResourceBlock::~PhysicalResourceBlock()
{
}

simTimeType
PhysicalResourceBlock::getUsedTime() const
{
    assure(slotLength-freeTime-nextPosition<wns::scheduler::slotLengthRoundingTolerance,"slotLength mismatch");
    return slotLength-freeTime;
}

simTimeType
PhysicalResourceBlock::getFreeTime() const
{
    assure(slotLength-freeTime-nextPosition<wns::scheduler::slotLengthRoundingTolerance,"slotLength mismatch");
    return freeTime;
}

simTimeType
PhysicalResourceBlock::getNextPosition() const
{
    return nextPosition;
}

bool
PhysicalResourceBlock::pduFitsInto(strategy::RequestForResource& request,
                                   MapInfoEntryPtr mapInfoEntry // <- must not contain compounds yet
    ) const
{
    // mapInfoEntry can contain compounds when in while loop:
    //assure(mapInfoEntry->compounds.empty(),"mapInfoEntry->compounds must be empty here");
    wns::service::phy::phymode::PhyModeInterfacePtr mapPhyModePtr = mapInfoEntry->phyModePtr;
    assure(mapPhyModePtr!=wns::service::phy::phymode::PhyModeInterfacePtr(),"phyModePtr==NULL");
    assure(scheduledCompounds.empty()
           || (*mapPhyModePtr == *phyModePtr),
           "all PhyModes must match on a (used) PhysicalResourceBlock: "<<*phyModePtr<<" != "<<*mapPhyModePtr);
    double dataRate = mapPhyModePtr->getDataRate();
    simTimeType compoundDuration = request.bits / dataRate;
    simTimeType startTime = this->nextPosition;
    simTimeType endTime = startTime + compoundDuration;
    return (endTime <= slotLength+wns::scheduler::slotLengthRoundingTolerance);
} // pduFitsInto (PhysicalResourceBlock)

int
PhysicalResourceBlock::getFreeBitsOnPhysicalResourceBlock(MapInfoEntryPtr mapInfoEntry) const
{
    wns::service::phy::phymode::PhyModeInterfacePtr mapPhyModePtr = mapInfoEntry->phyModePtr;
    assure(mapPhyModePtr!=wns::service::phy::phymode::PhyModeInterfacePtr(),"phyModePtr==NULL");
    assure(scheduledCompounds.empty()
           || (*mapPhyModePtr == *phyModePtr),
           "all PhyModes must match on a (used) PhysicalResourceBlock: "<<*phyModePtr<<" != "<<*mapPhyModePtr);
    double dataRate = mapPhyModePtr->getDataRate();
    return (freeTime+wns::scheduler::slotLengthRoundingTolerance) * dataRate;
} // getFreeBitsOnPhysicalResourceBlock

bool
PhysicalResourceBlock::addCompound(simTimeType compoundDuration,
                                   wns::scheduler::ConnectionID connectionID,
                                   wns::scheduler::UserID _userID,
                                   wns::ldk::CompoundPtr compoundPtr,
                                   wns::service::phy::phymode::PhyModeInterfacePtr _phyModePtr,
                                   wns::Power _txPower,
                                   wns::service::phy::ofdma::PatternPtr _pattern
    )
{
    simTimeType startTime = this->nextPosition;
    simTimeType endTime = startTime + compoundDuration;
    if (endTime > slotLength+wns::scheduler::slotLengthRoundingTolerance) return false; // not successful
    assure(endTime <= slotLength+wns::scheduler::slotLengthRoundingTolerance,
           "end time of the compound="<<endTime<<" exceeds slotLength="<<slotLength);
    int predecessors = scheduledCompounds.size();
    // this can happen if DSAStrategy.oneUserOnOneSubChannel=False: no implementation yet
    assure((predecessors==0)||(txPower == _txPower),
           "mismatch: txPower="<<txPower<<"!="<<_txPower<<" please use DSAStrategy.oneUserOnOneSubChannel=True");
    txPower = _txPower; // all compounds must have the same power
    // check if users match (need oneUserOnOneSubChannel for that)
    // we cannot do this here, because in the UL SchedulingMap done by UT.RS-TX the userID is myself
    //if (userID!=NULL) { // && oneUserOnOneSubChannel) {
    //    assure(_userID==userID,
    //           "user mismatch: "<<_userID->getName()<<" != "<<userID->getName());
    //}
    if (predecessors>0) { // && oneUserOnOneSubChannel) {
        wns::scheduler::UserID otherUserID = scheduledCompounds.begin()->userID;
        assure(_userID==otherUserID,
               "user mismatch: "<<_userID->getName()<<" != "<<userID->getName());
    }
    if (userID==NULL)
    { // master schedulers set this; slave schedulers get this already set.
        userID = _userID; // all compounds must have the same user; oneUserOnOneSubChannel = True ?
    }
    assure((predecessors==0)||(phyModePtr == _phyModePtr),
           "phyModePtr mismatch: "<<*phyModePtr<<"!="<<*_phyModePtr);
    phyModePtr = _phyModePtr; // all compounds should have the same phyMode
    antennaPattern = _pattern; // all compounds should have the same antenna pattern
    SchedulingCompound newScheduledCompound(this->subChannelIndex,
                                            this->timeSlotIndex,
                                            this->beamIndex,
                                            startTime,
                                            endTime,
                                            connectionID,
                                            _userID,
                                            compoundPtr,
                                            phyModePtr,
                                            txPower,
                                            _pattern
        );
    this->scheduledCompounds.push_back(newScheduledCompound);
    this->nextPosition += compoundDuration;
    assure(fabs(nextPosition-endTime)<wns::scheduler::slotLengthRoundingTolerance,"nextPosition!=endTime");
    this->freeTime     -= compoundDuration;
    assure(freeTime>=-wns::scheduler::slotLengthRoundingTolerance,"freeTime="<<freeTime);
    // assure(freeTime>=0.0,"freeTime="<<freeTime); // fails due to double inaccuracies
    // this method cannot count SchedulingMap::numberOfCompounds itself
    return true;
} // PhysicalResourceBlock::addCompound

bool
PhysicalResourceBlock::addCompound(strategy::RequestForResource& request,
                                   MapInfoEntryPtr mapInfoEntry, // <- must not contain compounds yet
                                   wns::ldk::CompoundPtr compoundPtr
    )
{
    // mapInfoEntry can contain compounds when in while loop:
    //assure(mapInfoEntry->compounds.empty(),"mapInfoEntry->compounds must be empty here");
    //assure(compoundPtr!=wns::ldk::CompoundPtr(),"compoundPtr==NULL"); // empty is allowed for uplink master scheduling
    //int compoundBits = compoundPtr->getLengthInBits();
    //assure(compoundBits==request.bits, "bits mismatch: "<<compoundBits<<" != "<<request.bits);
    // ^ in the UL the real bits may be less than the requested bits.
    //assure(compoundBits<=request.bits, "bits mismatch: "<<compoundBits<<" != "<<request.bits);
    wns::service::phy::phymode::PhyModeInterfacePtr mapPhyModePtr = mapInfoEntry->phyModePtr;
    double dataRate = mapPhyModePtr->getDataRate();
    simTimeType compoundDuration = request.bits / dataRate;
    wns::node::Interface* compoundUserID = mapInfoEntry->user;
    // check if users match (need oneUserOnOneSubChannel for that)
    // we cannot do this here, because in the UL SchedulingMap done by UT.RS-TX the userID is myself
    //if (userID!=NULL) { // && oneUserOnOneSubChannel) {
    //    assure(compoundUserID==userID,
    //           "user mismatch: "<<compoundUserID->getName()<<" != "<<userID->getName());
    //}
    // assure(compoundUserID==request.user) sometimes fails even with RN2 != RN2
    // Reason: user mismatch: RN2 != RN2, 7 != 5
    // in this case make a string comparison :-(
    //assure(compoundUserID->isEqual(request.user) || compoundUserID->getName().compare(mapInfoEntry->user->getName())==0,
    //       "user mismatch: "<<compoundUserID->getName()<<" != "<<mapInfoEntry->user->getName()<<", "<<compoundUserID->getNodeID()<<" != "<<request.user->getNodeID());
    wns::Power txPowerInMap = mapInfoEntry->txPower;
    wns::scheduler::ConnectionID connectionID = request.cid;
    wns::service::phy::ofdma::PatternPtr patternInMap = mapInfoEntry->pattern; // antenna pattern (grouping)

    return addCompound(compoundDuration,
                       connectionID,
                       compoundUserID,
                       compoundPtr,
                       mapPhyModePtr,
                       txPowerInMap,
                       patternInMap
        );
} // addCompound

// dumpContents(): machine-readable format (table for Matlab,Gnuplot,etc)
std::string
PhysicalResourceBlock::dumpContents(const std::string& prefix) const
{
    std::stringstream s;
    s.setf(std::ios::fixed,std::ios::floatfield);   // floatfield set to fixed
    s.precision(4);
    //s << prefix << subChannelIndex << "\t" << beamIndex;
    s << prefix;
    if (phyModePtr != PhyModePtr())
    {
        s << phyModePtr->getBitsPerSymbol() << "\t";
    }
    else
    {
        s << "?" << "\t";
    }

    s  << txPower.get_dBm() << "\t"
       << nextPosition/slotLength << "\t";
    if ( nextPosition>0.0 ) // not empty
    {
        s << scheduledCompounds.size() << "\t";
        if (scheduledCompounds.size() > 0)
        {
            int totalbits = 0;

            s << scheduledCompounds.begin()->userID->getName() << "\t";
            for ( ScheduledCompoundsList::const_iterator iter = scheduledCompounds.begin(); iter != scheduledCompounds.end(); ++iter )
            {
                s << iter->connectionID << "(" << iter->compoundPtr->getLengthInBits() << "),";
                totalbits += iter->compoundPtr->getLengthInBits();
            }
            s << "total (" << totalbits << ")";
        }
    } else {
        s << "0\t0";
    }
    s << std::endl;
    return s.str();
} // dumpContents

// doToString(): human-readable format
std::string
PhysicalResourceBlock::toString() const
{
    std::stringstream s;
    s.setf(std::ios::fixed,std::ios::floatfield);   // floatfield set to fixed
    s.precision(1);
    s << "PhysicalResourceBlock(";
    s << "#"<<subChannelIndex<<"."<<timeSlotIndex<<"."<<beamIndex;
    if ( nextPosition>0.0 ) { // not empty
        if (userID!=NULL)
            s << ", user="<<userID->getName();
        s << ", free=" << freeTime*1e6 << "us";
        s << ", next=" << nextPosition*1e6 << "us";
        s << ", used=" << 100.0*nextPosition/slotLength << "%";
        s << ", compounds=" << scheduledCompounds.size();
        s << ")";
        if (scheduledCompounds.size()>0)
        {
            s << ":"<<std::endl;
            for ( ScheduledCompoundsList::const_iterator iter = scheduledCompounds.begin(); iter != scheduledCompounds.end(); ++iter )
            {
                s << "  " << iter->toString();
                if (iter!=scheduledCompounds.end()) s << std::endl;
            }
        }
    } else if (phyModePtr!=wns::service::phy::phymode::PhyModeInterfacePtr()) {
        // this is a master schedulingMap (from BS to UT). Compounds have been deleted, values reset
        if (userID!=NULL)
            s << ", user="<<userID->getName();
        s << ", phyMode="<<*phyModePtr;
        s << ", txPower="<<txPower;
        s << ")";
    } else {
        s << ", unused)";
    }
    s << std::endl;
    return s.str();
} // toString

bool
PhysicalResourceBlock::isEmpty() const
{
    // nextPosition can be 0.0 in a master schedulingMap, although phyMode and user is set
    // so this && check is necessary in the master schedulers
    return ((nextPosition==0.0) &&
            (phyModePtr==wns::service::phy::phymode::PhyModeInterfacePtr()));
}

wns::scheduler::UserID
PhysicalResourceBlock::getUserID() const
{
    // In UL slave scheduler, the userIDs are different! (UT vs BS)
    //assure(userID==NULL || scheduledCompounds.size()==0 || (scheduledCompounds.begin()->userID == userID),
    //       "userID mismatch: userID="<<userID->getName());
    return userID;
    //return scheduledCompounds.begin()->userID;
}

wns::Power
PhysicalResourceBlock::getTxPower() const
{
    return txPower;
}

void
PhysicalResourceBlock::deleteCompounds()
{
    scheduledCompounds.clear();
} // deleteCompounds

void
PhysicalResourceBlock::grantFullResources()
{
    // extend UL resource size to full length, so that UT can make use of it.
    if (!isEmpty())
    {
        // with these values the resourceUsage statistics counts the full resource:
        nextPosition = slotLength;
        freeTime = 0.0;
    }
}

void
PhysicalResourceBlock::processMasterMap()
{
    // extend UL resource size to full length, so that UT can make use of it.
    if (!isEmpty())
    {
        scheduledCompounds.clear();
        // with these values the DSASlave strategy can put new UL packets into it:
        nextPosition = 0.0;
        freeTime = slotLength;
    }
    assure(scheduledCompounds.size() == 0,"scheduledCompounds is not empty but must be");
}

bool PhysicalResourceBlock::hasResourcesForUser(wns::scheduler::UserID user) const
{
    return (user == userID);
}

/**************************************************************/
SchedulingTimeSlot::SchedulingTimeSlot()
    : subChannelIndex(0),
      timeSlotIndex(0),
      numberOfBeams(0),
      slotLength(0.0),
      timeSlotStartTime(0.0)
{
}

SchedulingTimeSlot::SchedulingTimeSlot(int _subChannel,
                                       int _timeSlot,
                                       int _numberOfBeams,
                                       simTimeType _slotLength
    )
    : subChannelIndex(_subChannel),
      timeSlotIndex(_timeSlot),
      numberOfBeams(_numberOfBeams),
      slotLength(_slotLength),
      timeSlotStartTime(_slotLength*_timeSlot),
      timeSlotIsUsable(true)
{
    for ( int beamIndex = 0; beamIndex < numberOfBeams; ++beamIndex )
    {
        // create one PhysicalResourceBlock object per beam (MIMO channel)
        PhysicalResourceBlock emptyPRB(subChannelIndex,timeSlotIndex,beamIndex,slotLength);
        // available as physicalResources[beamIndex]
        physicalResources.push_back(emptyPRB); // object copied
    }
}

SchedulingTimeSlot::~SchedulingTimeSlot()
{
}

simTimeType
SchedulingTimeSlot::getUsedTime() const
{
    //if (!IsUsable) return 0.0; // must be different result than getFreeTime()
    simTimeType usedTime = 0.0;
    for(int beamIndex=0; beamIndex<numberOfBeams; beamIndex++)
    {
        usedTime += physicalResources[beamIndex].getUsedTime();
    }
    return usedTime;
}

simTimeType
SchedulingTimeSlot::getFreeTime() const
{
    //if (!IsUsable) return 0.0; // must be different result than getUsedTime()
    simTimeType freeTime = 0.0;
    for(int beamIndex=0; beamIndex<numberOfBeams; beamIndex++)
    {
        freeTime += physicalResources[beamIndex].getFreeTime();
    }
    return freeTime;
}

bool
SchedulingTimeSlot::isEmpty() const
{
    for(int beamIndex=0; beamIndex<numberOfBeams; beamIndex++)
    {
        if (!physicalResources[beamIndex].isEmpty()) return false;
    }
    return true;
}

bool
SchedulingTimeSlot::pduFitsInto(strategy::RequestForResource& request,
                                MapInfoEntryPtr mapInfoEntry // <- must not contain compounds yet
    ) const
{
    //if (!IsUsable) return false;
    // is it correct to ask like this?
    // or do we have to loop over all beams?
    int beam = mapInfoEntry->beam;
    assure(beam>=0 && beam<numberOfBeams,"beam="<<beam<<" is out of bounds");
    return physicalResources[beam].pduFitsInto(request,mapInfoEntry);
} // pduFitsInto (ResourceBlock)

wns::scheduler::UserID
SchedulingTimeSlot::getUserID() const
{
    for(int beamIndex=0; beamIndex<numberOfBeams; beamIndex++)
    {
        wns::scheduler::UserID userID = physicalResources[beamIndex].getUserID();
        if (userID!=NULL)
            return userID;
    }
    return NULL;
}

wns::Power
SchedulingTimeSlot::getTxPower() const
{
    wns::Power txPower;
    for(int beamIndex=0; beamIndex<numberOfBeams; beamIndex++)
    {
        txPower = physicalResources[beamIndex].getTxPower();
        if (txPower!=wns::Power())
            return txPower;
    }
    return txPower;
}

// dumpContents(): machine-readable format (table for Matlab,Gnuplot,etc)
std::string
SchedulingTimeSlot::dumpContents(const std::string& prefix) const
{
    std::stringstream s;
    for(int beamIndex=0; beamIndex<numberOfBeams; beamIndex++)
    {
        std::stringstream p;
        p << prefix << beamIndex << "\t";
        if (timeSlotIsUsable) {
            s << physicalResources[beamIndex].dumpContents(p.str());
        } else {
            s << prefix << beamIndex << "\t" << "LOCKED" << std::endl;
        }
    }
    return s.str();
}

// doToString(): human-readable format
std::string
SchedulingTimeSlot::toString() const
{
    std::stringstream s;
    if (timeSlotIsUsable) {
        for(int beamIndex=0; beamIndex<numberOfBeams; beamIndex++)
        {
            s << physicalResources[beamIndex].toString();
        }
    } else {
        s << "TimeSlot(#"<<timeSlotIndex<<"): locked/unusable" << std::endl;
    }
    return s.str();
}

void
SchedulingTimeSlot::deleteCompounds()
{
    for(int beamIndex=0; beamIndex<numberOfBeams; beamIndex++)
    {
        physicalResources[beamIndex].deleteCompounds();
    }
}

void
SchedulingTimeSlot::grantFullResources()
{
    for(int beamIndex=0; beamIndex<numberOfBeams; beamIndex++)
    {
        physicalResources[beamIndex].grantFullResources();
    }
}

void
SchedulingTimeSlot::processMasterMap()
{
    for(int beamIndex=0; beamIndex<numberOfBeams; beamIndex++)
    {
        physicalResources[beamIndex].processMasterMap();
    }
}

bool
SchedulingTimeSlot::hasResourcesForUser(wns::scheduler::UserID user) const
{
    for(int beamIndex=0; beamIndex<numberOfBeams; beamIndex++)
    {
        if (physicalResources[beamIndex].hasResourcesForUser(user))
            return true;
    }
    return false;
}

/**************************************************************/

SchedulingSubChannel::SchedulingSubChannel()
    : subChannelIndex(0),
      numberOfBeams(0),
      slotLength(0.0),
      subChannelIsUsable(true)
{
}

SchedulingSubChannel::SchedulingSubChannel(int _subChannelIndex, int _numberOfTimeSlots, int _numberOfBeams, simTimeType _slotLength)
    : subChannelIndex(_subChannelIndex),
      numberOfTimeSlots(_numberOfTimeSlots),
      numberOfBeams(_numberOfBeams),
      slotLength(_slotLength),
      subChannelIsUsable(true)
{
    for ( int timeSlotIndex = 0; timeSlotIndex < numberOfTimeSlots; ++timeSlotIndex )
    {
        SchedulingTimeSlotPtr emptyTimeSlotPtr // SmartPtr...
            = SchedulingTimeSlotPtr(new SchedulingTimeSlot(subChannelIndex,timeSlotIndex,numberOfBeams,slotLength));
        temporalResources.push_back(emptyTimeSlotPtr); // SmartPtr copied
    }
}

SchedulingSubChannel::~SchedulingSubChannel()
{
    temporalResources.clear();
}

// dumpContents(): machine-readable format (table for Matlab,Gnuplot,etc)
std::string
SchedulingSubChannel::dumpContents(const std::string& prefix) const
{
    std::stringstream s;
    for ( int timeSlotIndex = 0; timeSlotIndex < numberOfTimeSlots; ++timeSlotIndex )
    {
        std::stringstream p;
        p << prefix << timeSlotIndex << "\t";
        if (subChannelIsUsable) {
            s << temporalResources[timeSlotIndex]->dumpContents(p.str());
        } else {
            s << prefix << timeSlotIndex << "\t" << "LOCKED" << std::endl;
        }
    }
    return s.str();
}

// doToString(): human-readable format
std::string
SchedulingSubChannel::toString() const
{
    std::stringstream s;
    if (subChannelIsUsable) {
        for ( int timeSlotIndex = 0; timeSlotIndex < numberOfTimeSlots; ++timeSlotIndex )
        {
            s << temporalResources[timeSlotIndex]->toString();
        }
    } else {
        s << "SubChannel(#"<<subChannelIndex<<"): locked/unusable" << std::endl;
    }
    return s.str();
}

simTimeType
SchedulingSubChannel::getUsedTime() const
{
    if (!subChannelIsUsable) return 0.0; // must be different result than getFreeTime()
    simTimeType usedTime = 0.0;
    for ( int timeSlotIndex = 0; timeSlotIndex < numberOfTimeSlots; ++timeSlotIndex )
    {
        usedTime += temporalResources[timeSlotIndex]->getUsedTime();
    }
    return usedTime;
}

simTimeType
SchedulingSubChannel::getFreeTime() const
{
    if (!subChannelIsUsable) return 0.0; // must be different result than getUsedTime()
    simTimeType freeTime = 0.0;
    for ( int timeSlotIndex = 0; timeSlotIndex < numberOfTimeSlots; ++timeSlotIndex )
    {
        freeTime += temporalResources[timeSlotIndex]->getFreeTime();
    }
    return freeTime;
}

bool
SchedulingSubChannel::pduFitsInto(strategy::RequestForResource& request,
                                  MapInfoEntryPtr mapInfoEntry // <- must not contain compounds yet
    ) const
{
    if (!subChannelIsUsable) return false;
    int timeSlot = mapInfoEntry->timeSlot;
    assure(timeSlot>=0 && timeSlot<numberOfTimeSlots,"timeSlot="<<timeSlot<<" is out of bounds");
    // is it correct to ask like this?
    // or do we have to loop over all beams?
    int beam = mapInfoEntry->beam;
    assure(beam>=0 && beam<numberOfBeams,"beam="<<beam<<" is out of bounds");
    return temporalResources[timeSlot]->pduFitsInto(request,mapInfoEntry);
} // pduFitsInto (SubChannel)

int
SchedulingSubChannel::getFreeBitsOnSubChannel(MapInfoEntryPtr mapInfoEntry) const
{
    if (!subChannelIsUsable) return 0;
    int timeSlot = mapInfoEntry->timeSlot;
    assure(timeSlot>=0 && timeSlot<numberOfTimeSlots,"timeSlot="<<timeSlot<<" is out of bounds");
    // is it correct to ask like this?
    // or do we have to loop over all beams?
    int beam = mapInfoEntry->beam;
    assure(beam>=0 && beam<numberOfBeams,"beam="<<beam<<" is out of bounds");
    return temporalResources[timeSlot]->physicalResources[beam].getFreeBitsOnPhysicalResourceBlock(mapInfoEntry);
} // getFreeBitsOnSubChannel

bool
SchedulingSubChannel::isEmpty() const
{
    for (int timeSlotIndex = 0; timeSlotIndex < numberOfTimeSlots; ++timeSlotIndex)
    {
        if (!temporalResources[timeSlotIndex]->isEmpty()) return false;
    }
    return true;
}

void
SchedulingSubChannel::deleteCompounds()
{
    for ( int timeSlotIndex = 0; timeSlotIndex < numberOfTimeSlots; ++timeSlotIndex )
    {
        temporalResources[timeSlotIndex]->deleteCompounds();
    }
}

void
SchedulingSubChannel::grantFullResources()
{
    for ( int timeSlotIndex = 0; timeSlotIndex < numberOfTimeSlots; ++timeSlotIndex )
    {
        temporalResources[timeSlotIndex]->grantFullResources();
    }
}

void
SchedulingSubChannel::processMasterMap()
{
    for ( int timeSlotIndex = 0; timeSlotIndex < numberOfTimeSlots; ++timeSlotIndex )
    {
        temporalResources[timeSlotIndex]->processMasterMap();
    }
}

bool SchedulingSubChannel::hasResourcesForUser(wns::scheduler::UserID user) const
{
    for ( int timeSlotIndex = 0; timeSlotIndex < numberOfTimeSlots; ++timeSlotIndex )
    {
        if (temporalResources[timeSlotIndex]->hasResourcesForUser(user))
        return true;
    }
    return false;
}

/**************************************************************/

SchedulingMap::SchedulingMap( simTimeType _slotLength, int _numberOfSubChannels, int _numberOfTimeSlots, int _numberOfBeams, int _frameNr )
    : frameNr(_frameNr),
      slotLength(_slotLength),
      numberOfSubChannels(_numberOfSubChannels),
      numberOfTimeSlots(_numberOfTimeSlots),
      numberOfBeams(_numberOfBeams),
      numberOfCompounds(0),
      resourceUsage(0.0)
{
    assure(numberOfSubChannels>0,"numberOfSubChannels="<<numberOfSubChannels);
    assure(slotLength>0.0,"slotLength="<<slotLength);
    for ( int subChannelIndex = 0; subChannelIndex < numberOfSubChannels; ++subChannelIndex )
    {
        SchedulingSubChannel subChannel(subChannelIndex,numberOfTimeSlots,numberOfBeams,slotLength);
        subChannels.push_back(subChannel); // object copied
    }
    //#include <WNS/logger/Logger.hpp>
    //wns::logger::Logger logger("WNS", "SchedulingMap");
} // SchedulingMap::SchedulingMap

SchedulingMap::~SchedulingMap()
{
    subChannels.clear();
}

bool
SchedulingMap::pduFitsInto(strategy::RequestForResource& request,
                           MapInfoEntryPtr mapInfoEntry // <- must not contain compounds yet
    ) const
{
    // mapInfoEntry can contain compounds when in while loop:
    //assure(mapInfoEntry->compounds.empty(),"mapInfoEntry->compounds must be empty here");
    int subChannelIndex = mapInfoEntry->subBand;
    //int timeSlot = mapInfoEntry->timeSlot;
    //int beam = mapInfoEntry->beam;
    return subChannels[subChannelIndex].pduFitsInto(request,mapInfoEntry);
    //return subChannels[subChannelIndex].temporalResources[timeSlot]->physicalResources[beam].pduFitsInto(request,mapInfoEntry);
} // pduFitsInto (SubChannel)

int
SchedulingMap::getFreeBitsOnSubChannel(MapInfoEntryPtr mapInfoEntry) const
{
    // mapInfoEntry can contain compounds when in while loop:
    //assure(mapInfoEntry->compounds.empty(),"mapInfoEntry->compounds must be empty here");
    int subChannelIndex = mapInfoEntry->subBand;
    //int timeSlot = mapInfoEntry->timeSlot;
    //int beam = mapInfoEntry->beam;
    return subChannels[subChannelIndex].getFreeBitsOnSubChannel(mapInfoEntry);
}

bool
SchedulingMap::addCompound(int subChannelIndex,
                           int timeSlot,
                           int beam,
                           simTimeType compoundDuration,
                           wns::scheduler::ConnectionID connectionID,
                           wns::scheduler::UserID userID,
                           wns::ldk::CompoundPtr compoundPtr,
                           wns::service::phy::phymode::PhyModeInterfacePtr phyModePtr,
                           wns::Power txPower,
                           wns::service::phy::ofdma::PatternPtr pattern
    )
{
    bool ok =
        subChannels[subChannelIndex].temporalResources[timeSlot]->physicalResources[beam].addCompound(
            compoundDuration,
            connectionID,
            userID,
            compoundPtr,
            phyModePtr,
            txPower,
            pattern
            );
    if (ok) numberOfCompounds++;
    return ok;
} // addCompound

bool
SchedulingMap::addCompound(strategy::RequestForResource& request,
                           MapInfoEntryPtr mapInfoEntry, // <- must not contain compounds yet
                           wns::ldk::CompoundPtr compoundPtr
    )
{
    // mapInfoEntry can contain compounds when in while loop:
    //assure(mapInfoEntry->compounds.empty(),"mapInfoEntry->compounds must be empty here");
    int subChannelIndex = mapInfoEntry->subBand;
    int timeSlot = mapInfoEntry->timeSlot;
    int beam = mapInfoEntry->beam;
    bool ok =
        subChannels[subChannelIndex].temporalResources[timeSlot]->physicalResources[beam].addCompound(
            request,
            mapInfoEntry,
            compoundPtr
            );
    if (ok) numberOfCompounds++;
    return ok;
} // addCompound

simTimeType
SchedulingMap::getNextPosition(int subChannel, int timeSlot, int beam) const
{
    assure(subChannel<numberOfSubChannels,"subChannel="<<subChannel<<" >= numberOfSubChannels="<<numberOfSubChannels);
    assure(timeSlot>=0 && timeSlot<numberOfTimeSlots,"timeSlot="<<timeSlot<<" >= numberOfTimeSlots="<<numberOfTimeSlots);
    assure(beam<numberOfBeams,"beam="<<beam<<" >= numberOfBeams="<<numberOfBeams);
    return subChannels[subChannel].temporalResources[timeSlot]->physicalResources[beam].getNextPosition();
}

bool
SchedulingMap::isEmpty() const
{
    for ( unsigned int subChannelIndex = 0; subChannelIndex < subChannels.size(); ++subChannelIndex )
    {
        if (!subChannels[subChannelIndex].isEmpty()) return false;
    }
    return true;
}

double
SchedulingMap::getResourceUsage()
{
    simTimeType totalUsedTime = getUsedTime();
    simTimeType totalTimeResources = slotLength * numberOfSubChannels * numberOfBeams * numberOfTimeSlots;
    double result = totalUsedTime / totalTimeResources;
    assure(numberOfSubChannels==subChannels.size(),"mismatch in numberOfSubChannels: "<<numberOfSubChannels<<" != "<<subChannels.size());
    assure((result >= -0.01)/*tolerance*/
           &&(result <= 1.01)
           , "Percentage of used resources="<<result<<" must be within [0..1] !"
           <<" totalUsedTime="<<totalUsedTime
           <<", totalTime="<<totalTimeResources
           <<", subChannels.size="<<subChannels.size());
    if (result<0.0) result=0.0;
    if (result>1.0) result=1.0;
    resourceUsage = result; // store result in member
    return result;
} // getResourceUsage

simTimeType
SchedulingMap::getUsedTime() const
{
    simTimeType usedTime = 0;
    assure(numberOfSubChannels==subChannels.size(),"mismatch in numberOfSubChannels: "<<numberOfSubChannels<<" != "<<subChannels.size());
    for ( unsigned int subChannelIndex = 0; subChannelIndex < subChannels.size(); ++subChannelIndex )
    {
        usedTime += subChannels[subChannelIndex].getUsedTime();
    }
    return usedTime;
} // getUsedTime

simTimeType
SchedulingMap::getFreeTime() const
{
    simTimeType leftoverTime = 0;
    assure(numberOfSubChannels==subChannels.size(),"mismatch in numberOfSubChannels: "<<numberOfSubChannels<<" != "<<subChannels.size());
    for ( unsigned int subChannelIndex = 0; subChannelIndex < subChannels.size(); ++subChannelIndex )
    {
        leftoverTime += subChannels[subChannelIndex].getFreeTime();
    }
    return leftoverTime;
} // getFreeTime

wns::Power
SchedulingMap::getRemainingPower(wns::Power totalPower, int timeSlot) const
{
    wns::Power remainingPower = totalPower;
    for(unsigned int subChannelIndex=0; subChannelIndex<numberOfSubChannels; subChannelIndex++)
    {
        // what is the right handling of MIMO? Do we count=add txPower per beam or do we assume this is all "one" power?
        //wns::Power usedTxPowerOnThisChannel = subChannels[subChannelIndex].txPower;
        // we assume that txPower is the same on all PRBs, so reading the first is sufficient:
        wns::Power usedTxPowerOnThisChannel = subChannels[subChannelIndex].temporalResources[timeSlot]->physicalResources[0/*first beam*/].txPower;
        // if we have no PDU allocated on this channel, just skip it.
        if (usedTxPowerOnThisChannel == wns::Power())
            continue;
        if (remainingPower<usedTxPowerOnThisChannel)
            return wns::Power(); // zero Watts;
        remainingPower -= usedTxPowerOnThisChannel;
    }
    // the counter totalRemainingPower should be used in the future:
    //assure(remainingPower == totalRemainingPower,"remainingPower="<<remainingPower<<" != totalRemainingPower="<<totalRemainingPower);
    return remainingPower;
} // getRemainingPower

wns::service::phy::phymode::PhyModeInterfacePtr
SchedulingMap::getPhyModeUsedInResource(int subChannelIndex, int timeSlot, int beam) const
{
    assure(subChannelIndex>=0 && subChannelIndex<numberOfSubChannels,"subChannelIndex="<<subChannelIndex);
    assure(timeSlot>=0 && timeSlot<numberOfTimeSlots,"numberOfTimeSlots="<<numberOfTimeSlots);
    assure(beam>=0 && beam < numberOfBeams,"beam="<<beam);
    return subChannels[subChannelIndex].temporalResources[timeSlot]->physicalResources[beam].phyModePtr;
}

wns::Power
SchedulingMap::getTxPowerUsedInResource(int subChannelIndex, int timeSlot, int beam) const
{
    assure(subChannelIndex>=0 && subChannelIndex<numberOfSubChannels,"subChannelIndex="<<subChannelIndex);
    assure(timeSlot>=0 && timeSlot<numberOfTimeSlots,"numberOfTimeSlots="<<numberOfTimeSlots);
    assure(beam>=0 && beam < numberOfBeams,"beam="<<beam);
    return subChannels[subChannelIndex].temporalResources[timeSlot]->physicalResources[beam].txPower;
}

void
SchedulingMap::maskOutSubChannels(const UsableSubChannelVector& usableSubChannels)
{
    //std::cout <<"maskOutSubChannels(#="<<usableSubChannels.size()<<"): usable="<<printBoolVector(usableSubChannels)<<std::endl;
    assure(usableSubChannels.size()>=numberOfSubChannels,"#usableSubChannels="<<usableSubChannels.size());
    for ( int subChannelIndex = 0; subChannelIndex < numberOfSubChannels; ++subChannelIndex )
    {
        bool before = subChannels[subChannelIndex].subChannelIsUsable;
        bool mask   = usableSubChannels[subChannelIndex];
        bool after  = before & mask; // 0&1=0
        subChannels[subChannelIndex].subChannelIsUsable = after;
    }
} // maskOutSubChannels

void
SchedulingMap::convertToMapInfoCollection(MapInfoCollectionPtr collection /*return value*/)
{
    // translate result into currentBurst to allow bursts.push_back(currentBurst)
    for ( SubChannelVector::iterator iterSubChannel = subChannels.begin();
          iterSubChannel != subChannels.end(); ++iterSubChannel)
    {
        SchedulingSubChannel& subChannel = *iterSubChannel;
        for ( SchedulingTimeSlotPtrVector::iterator iterTimeSlot = subChannel.temporalResources.begin();
              iterTimeSlot != subChannel.temporalResources.end(); ++iterTimeSlot)
        {
            SchedulingTimeSlotPtr timeSlotPtr = *iterTimeSlot;
            for ( PhysicalResourceBlockVector::iterator iterPRB = timeSlotPtr->physicalResources.begin();
                  iterPRB != timeSlotPtr->physicalResources.end(); ++iterPRB)
            {
                UserID lastScheduledUserID = NULL;
                MapInfoEntryPtr currentBurst;
                double currentBurstStartTime = 0.0;
                while ( !iterPRB->scheduledCompounds.empty() )
                {
                    SchedulingCompound compound = iterPRB->scheduledCompounds.front(); // .front()
                    iterPRB->scheduledCompounds.pop_front(); // pop_front()
                    if ( compound.userID != lastScheduledUserID ) // new User, starts new Burst
                    {
                        //MESSAGE_SINGLE(NORMAL, logger, "New compund of cid=" << iterCompound.userID->getName());
                        // new user -> new newburst=new map entry
                        currentBurst = MapInfoEntryPtr(new MapInfoEntry());
                        currentBurst->start          = compound.startTime;
                        currentBurst->end            = compound.startTime; // intentionally not endTime;
                        currentBurst->user           = compound.userID;
                        currentBurst->subBand        = compound.subChannel;
                        currentBurst->timeSlot       = compound.timeSlot;
                        currentBurst->beam           = compound.beam;
                        currentBurst->txPower        = compound.txPower;
                        currentBurst->phyModePtr     = compound.phyModePtr;
                        //currentBurst->estimatedCandI = ? how to get it here?
                        collection->push_back(currentBurst);
                    }
                    //bursts.back()->end += iterCompound.compoundDuration;
                    simTimeType compoundDuration = compound.getCompoundDuration();
                    currentBurst->end     += compoundDuration;
                    currentBurstStartTime += compoundDuration;
                    //compound gehort zum selben connection fuege hinten ein
                    collection->back()->compounds.push_back(compound.compoundPtr);

                    // inherited from Strategy.cpp; calls callback():
                    // compoundReady() // cannot do this here.
                    lastScheduledUserID = compound.userID;
                } // end while ( !iterSubChannel->scheduledCompounds.empty() )
            } // end for ( beams )
        } // end for ( timeSlots )
    } // end for ( SubChannels )
} // convertToMapInfoCollection

// this is called by the UL master scheduler, because there are no "real" compounds (just fakes).
void
SchedulingMap::deleteCompounds()
{
    for ( int subChannelIndex = 0; subChannelIndex < numberOfSubChannels; ++subChannelIndex )
    {
        subChannels[subChannelIndex].deleteCompounds();
    }
    numberOfCompounds = 0;
}

void
SchedulingMap::grantFullResources()
{
    for ( int subChannelIndex = 0; subChannelIndex < numberOfSubChannels; ++subChannelIndex )
    {
        subChannels[subChannelIndex].grantFullResources();
    }
}

void
SchedulingMap::processMasterMap()
{
    for ( int subChannelIndex = 0; subChannelIndex < numberOfSubChannels; ++subChannelIndex )
    {
        subChannels[subChannelIndex].processMasterMap();
    }
}

bool
SchedulingMap::hasResourcesForUser(wns::scheduler::UserID user) const
{
    for ( int subChannelIndex = 0; subChannelIndex < numberOfSubChannels; ++subChannelIndex )
    {
        if (subChannels[subChannelIndex].hasResourcesForUser(user))
            return true;
    }
    return false;
}

// toString(): human-readable format (incuding resourceUsage)
std::string
SchedulingMap::toString()
{
    std::stringstream s;
    s.setf(std::ios::fixed,std::ios::floatfield);   // floatfield set to fixed
    s.precision(1);
    assure(numberOfSubChannels==subChannels.size(),"numberOfSubChannels="<<numberOfSubChannels<<" != subChannels.size()="<<subChannels.size());
    if (frameNr>=0) { // valid frameNr
        s << "SchedulingMap(frame="<<frameNr<<": "<<numberOfSubChannels<<"x"<<slotLength*1e6<<"us): ";
    } else {
        s << "SchedulingMap("<<numberOfSubChannels<<"x"<<slotLength*1e6<<"us): ";
    }
    double resourceUsage = this->getResourceUsage(); // getResourceUsage() is not const
    if (this->isEmpty()) {
        s << "empty." << std::endl;
    } else {
        s << resourceUsage*100.0 << "% full." << std::endl;
        for ( int subChannelIndex = 0; subChannelIndex < numberOfSubChannels; ++subChannelIndex )
        {
            s << subChannels[subChannelIndex].toString();
        }
    }
    return s.str();
} // toString

// doToString(): human-readable format
std::string
SchedulingMap::doToString() const
{
    std::stringstream s;
    s.setf(std::ios::fixed,std::ios::floatfield);   // floatfield set to fixed
    s.precision(1);
    assure(numberOfSubChannels==subChannels.size(),"numberOfSubChannels="<<numberOfSubChannels<<" != subChannels.size()="<<subChannels.size());
    if (frameNr>=0) { // valid frameNr
        s << "SchedulingMap(frame="<<frameNr<<": "<<numberOfSubChannels<<"x"<<slotLength*1e6<<"us): ";
    } else {
        s << "SchedulingMap("<<numberOfSubChannels<<"x"<<slotLength*1e6<<"us): ";
    }
    if (this->isEmpty()) {
        s << "empty." << std::endl;
    } else {
        for ( int subChannelIndex = 0; subChannelIndex < numberOfSubChannels; ++subChannelIndex )
        {
            s << subChannels[subChannelIndex].toString();
        }
    }
    return s.str();
} // doToString

// dumpContents(): machine-readable format (table for Matlab,Gnuplot,etc)
std::string
SchedulingMap::dumpContents(const std::string& prefix) const
{
    std::stringstream s;
    //s.setf(std::ios::fixed,std::ios::floatfield);   // floatfield set to fixed
    //s.precision(1);
    assure(numberOfSubChannels==subChannels.size(),"numberOfSubChannels="<<numberOfSubChannels<<" != subChannels.size()="<<subChannels.size());
    for ( int subChannelIndex = 0; subChannelIndex < numberOfSubChannels; ++subChannelIndex )
    {
        std::stringstream p;
        p << prefix << frameNr << "\t" << subChannelIndex << "\t";
        s << subChannels[subChannelIndex].dumpContents(p.str());
    }
    return s.str();
} // dumpContents

// static (does not need an object of this class)
void
SchedulingMap::writeHeaderToFile(std::ofstream& f)
{
    if (f.is_open()) {
        f << "##### SchedulingMap over time #####" << std::endl;
        //f << "# numberOfSubChannels="<<numberOfSubChannels << std::endl;
        //f << "# numberOfTimeSlots="<<numberOfTimeSlots << std::endl;
        //f << "# numberOfBeams="<<numberOfBeams << std::endl;
        //f << "# slotLength="<<slotLength << std::endl;
        f << "# (time[s]) frameNr subChannel timeSlot stream/beam bits/symbol txPower[dBm] filled% #compounds userID cidList(#bits), totalbits" << std::endl;
    } else {
        throw wns::Exception("cannot write to file");
    }
}

void
SchedulingMap::writeToFile(std::ofstream& f, const std::string& prefix) const
{
    if (f.is_open()) {
        f << dumpContents(prefix);
    } else {
        throw wns::Exception("cannot write to file");
    }
}

void
SchedulingMap::writeFile(std::string fileName) const
{
    //std::ofstream *file = new std::ofstream(fileName.c_str());
    //(*file) << doToString();
    //if (*file) file->close();
    std::ofstream file(fileName.c_str(),std::ios_base::app); // append
    if (file.is_open())
    {
        writeToFile(file,"");
        file.close();
    } else {
        throw wns::Exception("cannot open file "+fileName);
    }
}
