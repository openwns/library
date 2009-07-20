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

using namespace wns::scheduler;

SchedulingCompound::SchedulingCompound()
: subChannel(0),
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

PhysicalResourceBlock::PhysicalResourceBlock(int _subChannelIndex, int _beam, simTimeType _slotLength)
  : subChannelIndex(_subChannelIndex),
    beamIndex(_beam),
    slotLength(_slotLength),
    freeTime(_slotLength),
    nextPosition(0.0),
    scheduledCompounds(),
    phyModePtr(),
    txPower()
{
}

PhysicalResourceBlock::~PhysicalResourceBlock()
{
}

std::string
PhysicalResourceBlock::toString() const
{
	std::stringstream s;
	s.setf(std::ios::fixed,std::ios::floatfield);   // floatfield set to fixed
	s.precision(1);
	s << "PhysicalResourceBlock(";
	s << "#"<<subChannelIndex<<"."<<beamIndex;
	if ( scheduledCompounds.size() > 0 ) {
	  s << ", freetime=" << freeTime*1e6 << "us";
	  s << ", nextPos=" << nextPosition*1e6 << "us";
	  s << ", used=" << 100.0*nextPosition/slotLength << "%";
	  s << ", compounds=" << scheduledCompounds.size();
	  s << ")";
	  if (scheduledCompounds.size()>0)
	  {
	    s << ":"<<std::endl;
	    for ( ScheduledCompoundsList::const_iterator iter = scheduledCompounds.begin(); iter != scheduledCompounds.end(); ++iter )
	      {
		s << "  " << iter->toString() << std::endl;
	      }
	  }
	} else {
	  s << ", unused)";
	}
	s << std::endl;
	return s.str();
} // toString

simTimeType
PhysicalResourceBlock::getFreeTime() const
{
  assure(slotLength-freeTime-nextPosition<wns::scheduler::slotLengthRoundingTolerance,"slotLength mismatch");
  return freeTime;
}

void
PhysicalResourceBlock::setNextPosition(simTimeType _nextPosition)
{
  nextPosition = _nextPosition;
}

simTimeType
PhysicalResourceBlock::getNextPosition() const
{
	return nextPosition;
}

bool
PhysicalResourceBlock::pduFitsIntoPhysicalResourceBlock(strategy::RequestForResource& request,
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
} // pduFitsIntoPhysicalResourceBlock

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
				   wns::scheduler::UserID userID,
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
	assure((predecessors==0)||(phyModePtr == _phyModePtr),
	       "phyModePtr mismatch: "<<*phyModePtr<<"!="<<*_phyModePtr);
	phyModePtr = _phyModePtr; // all compounds should have the same phyMode
	antennaPattern = _pattern; // all compounds should have the same antenna pattern
	SchedulingCompound newScheduledCompound(this->subChannelIndex,
						this->beamIndex,
						startTime,
						endTime,
						connectionID,
						userID,
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
  wns::node::Interface* userID = mapInfoEntry->user;
  // assure(userID==request.user) sometimes fails even with RN2 != RN2
  // Reason: user mismatch: RN2 != RN2, 7 != 5
  // in this case make a string comparison :-(
  assure(userID->isEqual(request.user) || userID->getName().compare(mapInfoEntry->user->getName())==0,
	 "user mismatch: "<<userID->getName()<<" != "<<mapInfoEntry->user->getName()<<", "<<userID->getNodeID()<<" != "<<request.user->getNodeID());
  wns::Power txPowerInMap = mapInfoEntry->txPower;
  wns::scheduler::ConnectionID connectionID = request.cid;
  wns::service::phy::ofdma::PatternPtr patternInMap = mapInfoEntry->pattern; // antenna pattern (grouping)

  return addCompound(compoundDuration,
		     connectionID,
		     userID,
		     compoundPtr,
		     mapPhyModePtr,
		     txPowerInMap,
		     patternInMap
		     );
} // addCompound

/**************************************************************/

SchedulingSubChannel::SchedulingSubChannel()
  : subChannelIndex(0),
    numberOfBeams(0),
    slotLength(0.0),
    subChannelIsUsable(true)
{
}

SchedulingSubChannel::SchedulingSubChannel(int _subChannelIndex, int _numberOfBeams, simTimeType _slotLength)
  : subChannelIndex(_subChannelIndex),
    numberOfBeams(_numberOfBeams),
    slotLength(_slotLength),
    subChannelIsUsable(true)
{
	for ( int beamIndex = 0; beamIndex < numberOfBeams; ++beamIndex )
	{
	    // create one PhysicalResourceBlock object per beam (MIMO channel)
	    PhysicalResourceBlock emptyPRB(subChannelIndex,beamIndex,slotLength);
	    // available as physicalResources[beamIndex]
	    physicalResources.push_back(emptyPRB); // object copied
	}
}

SchedulingSubChannel::~SchedulingSubChannel()
{
	physicalResources.clear();
}

std::string
SchedulingSubChannel::toString() const
{
	std::stringstream s;
	if (subChannelIsUsable) {
	  for(int beamIndex=0; beamIndex<numberOfBeams; beamIndex++)
	  {
	    s << physicalResources[beamIndex].toString();
	  }
	} else {
	  s << "SubChannel(#"<<subChannelIndex<<"): locked/unusable";
	}
	return s.str();
}

simTimeType
SchedulingSubChannel::getFreeTime() const
{
  if (!subChannelIsUsable) return 0.0;
  simTimeType freeTime = 0.0;
  for(int beamIndex=0; beamIndex<numberOfBeams; beamIndex++)
  {
    freeTime += physicalResources[beamIndex].getFreeTime();
  }
  return freeTime;
}

bool
SchedulingSubChannel::pduFitsIntoSubChannel(strategy::RequestForResource& request,
					    MapInfoEntryPtr mapInfoEntry // <- must not contain compounds yet
					    ) const
{
  if (!subChannelIsUsable) return false;
  // is it correct to ask like this?
  // or do we have to loop over all beams?
  int beam = mapInfoEntry->beam;
  assure(beam>=0 && beam<numberOfBeams,"beam="<<beam<<" is out of bounds");
  return physicalResources[beam].pduFitsIntoPhysicalResourceBlock(request,mapInfoEntry);
} // pduFitsIntoSubChannel

int
SchedulingSubChannel::getFreeBitsOnSubChannel(MapInfoEntryPtr mapInfoEntry) const
{
  if (!subChannelIsUsable) return 0;
  // is it correct to ask like this?
  // or do we have to loop over all beams?
  int beam = mapInfoEntry->beam;
  assure(beam>=0 && beam<numberOfBeams,"beam="<<beam<<" is out of bounds");
  return physicalResources[beam].getFreeBitsOnPhysicalResourceBlock(mapInfoEntry);
} // getFreeBitsOnSubChannel

/**************************************************************/

SchedulingMap::SchedulingMap( simTimeType _slotLength, int _numberOfSubChannels, int _numberOfBeams, int _frameNr )
//SchedulingMap::SchedulingMap(const simTimeType& _slotLength, const int& _subChannels)
  : frameNr(_frameNr),
    slotLength(_slotLength),
    numberOfSubChannels(_numberOfSubChannels),
    numberOfBeams(_numberOfBeams),
    numberOfCompounds(0),
    resourceUsage(0.0)
{
	assure(numberOfSubChannels>0,"numberOfSubChannels="<<numberOfSubChannels);
	assure(slotLength>0.0,"slotLength="<<slotLength);
	for ( int subChannelIndex = 0; subChannelIndex < numberOfSubChannels; ++subChannelIndex )
	{
	    SchedulingSubChannel subChannel(subChannelIndex,numberOfBeams,slotLength);
	    subChannels.push_back(subChannel); // object copied
	}
	//std::cout << "SchedulingMap(slotlength=" << slotLength << ", subchannels=" << numberOfSubChannels <<")"<< std::endl;
} // SchedulingMap::SchedulingMap

SchedulingMap::~SchedulingMap()
{
	subChannels.clear();
}

bool
SchedulingMap::pduFitsIntoSubChannel(strategy::RequestForResource& request,
				     MapInfoEntryPtr mapInfoEntry // <- must not contain compounds yet
				     ) const
{
  // mapInfoEntry can contain compounds when in while loop:
  //assure(mapInfoEntry->compounds.empty(),"mapInfoEntry->compounds must be empty here");
  int subChannelIndex = mapInfoEntry->subBand;
  //int beam = mapInfoEntry->beam;
  return subChannels[subChannelIndex].pduFitsIntoSubChannel(request,mapInfoEntry);
} // pduFitsIntoSubChannel

int
SchedulingMap::getFreeBitsOnSubChannel(MapInfoEntryPtr mapInfoEntry) const
{
  // mapInfoEntry can contain compounds when in while loop:
  //assure(mapInfoEntry->compounds.empty(),"mapInfoEntry->compounds must be empty here");
  int subChannelIndex = mapInfoEntry->subBand;
  //int beam = mapInfoEntry->beam;
  return subChannels[subChannelIndex].getFreeBitsOnSubChannel(mapInfoEntry);
}

bool
SchedulingMap::addCompound(int subChannelIndex,
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
  //std::cout << "SchedulingMap::addcompound to subch=" << subChannelNum << ", cid=" << connectionID << std::endl;
  bool ok =
  subChannels[subChannelIndex].physicalResources[beam].addCompound(
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
  int beam = mapInfoEntry->beam;
  bool ok =
  subChannels[subChannelIndex].physicalResources[beam].addCompound(
	request,
	mapInfoEntry,
	compoundPtr
	);
  if (ok) numberOfCompounds++;
  return ok;
} // addCompound

simTimeType
SchedulingMap::getNextPosition(int subChannel, int beam) const
{
	assure(subChannel<numberOfSubChannels,"subChannel="<<subChannel<<" >= numberOfSubChannels="<<numberOfSubChannels);
	assure(beam<numberOfBeams,"beam="<<beam<<" >= numberOfBeams="<<numberOfBeams);
	return subChannels[subChannel].physicalResources[beam].getNextPosition();
}

double
SchedulingMap::getResourceUsage()
{
	if (resourceUsage > 0.0) return resourceUsage; // was already precalculated
	simTimeType leftoverTime = getFreeTime();
	simTimeType totalTimeResources = slotLength * numberOfSubChannels * numberOfBeams;
	double result = (totalTimeResources - leftoverTime) / totalTimeResources;
	assure(numberOfSubChannels==subChannels.size(),"mismatch in numberOfSubChannels: "<<numberOfSubChannels<<" != "<<subChannels.size());
	assure(result >= -0.01/*tolerance*/, "Percentage of used resources must not be negative!"
	       <<" leftTime="<<leftoverTime
	       <<", totalTime="<<totalTimeResources
	       <<", subChannels.size="<<subChannels.size());
	if (result<0.0) result=0.0;
	resourceUsage = result; // store result in member
	return result;
} // getResourceUsage

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
SchedulingMap::getRemainingPower(wns::Power totalPower) const
{
	wns::Power remainingPower = totalPower;
	for(unsigned int subChannelIndex=0; subChannelIndex<numberOfSubChannels; subChannelIndex++)
	{
		// what is the right handling of MIMO? Do we count=add txPower per beam or do we assume this is all "one" power?
		//wns::Power usedTxPowerOnThisChannel = subChannels[subChannelIndex].txPower;
		// we assume that txPower is the same on all PRBs, so reading the first is sufficient:
		wns::Power usedTxPowerOnThisChannel = subChannels[subChannelIndex].physicalResources[0/*first beam*/].txPower;
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
}

void
SchedulingMap::convertToMapInfoCollection(MapInfoCollectionPtr collection /*return value*/)
{
	// translate result into currentBurst to allow bursts.push_back(currentBurst)
	for ( SubChannelVector::iterator iterSubChannel = subChannels.begin();
	      iterSubChannel != subChannels.end(); ++iterSubChannel)
	{
	    SchedulingSubChannel& subChannel = *iterSubChannel;
	    for ( PhysicalResourceBlockVector::iterator iterPRB = subChannel.physicalResources.begin();
		iterPRB != subChannel.physicalResources.end(); ++iterPRB)
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
	} // end for ( SubChannels )
} // convertToMapInfoCollection

std::string
SchedulingMap::toString()
{
	std::stringstream s;
	s.setf(std::ios::fixed,std::ios::floatfield);   // floatfield set to fixed
	s.precision(1);
	assure(numberOfSubChannels==subChannels.size(),"numberOfSubChannels="<<numberOfSubChannels<<" != subChannels.size()="<<subChannels.size());
	assure(std::fabs(slotLength-subChannels[0].slotLength)<1e-6,"mismatch in slotLength="<<slotLength);
	if (frameNr>=0) { // valid frameNr
	  s << "SchedulingMap(frame="<<frameNr<<": "<<numberOfSubChannels<<"x"<<slotLength*1e6<<"us): ";
	} else {
	  s << "SchedulingMap("<<numberOfSubChannels<<"x"<<slotLength*1e6<<"us): ";
	}
	double resourceUsage = this->getResourceUsage(); // getResourceUsage() is not const
	if (resourceUsage < slotLengthRoundingTolerance) {
	  s << "empty." << std::endl;
	} else {
	  s << resourceUsage*100.0 << "% full." << std::endl;
	  for ( int subChannelIndex = 0; subChannelIndex < numberOfSubChannels; ++subChannelIndex )
	  {
	    s << subChannels[subChannelIndex].toString();
	  }
	}
	return s.str();
}


