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

#include <WNS/scheduler/strategy/dsastrategy/DSAStrategy.hpp>
#include <WNS/scheduler/strategy/Strategy.hpp>
#include <string>

using namespace wns::scheduler;
using namespace wns::scheduler::strategy;
using namespace wns::scheduler::strategy::dsastrategy;

DSAStrategy::DSAStrategy(const wns::pyconfig::View& config)
	: logger(config.get("logger")),
	  oneUserOnOneSubChannel(config.get<bool>("oneUserOnOneSubChannel")),
	  adjacentSubchannelsOnUplink(config.get<bool>("adjacentSubchannelsOnUplink")),
	  highestDataRatePerSubChannel(0.0),
	  phyModeMapper(NULL),
	  myUserID(NULL),
	  dsastrategyName(config.get<std::string>("nameInDSAStrategyFactory"))
{
    MESSAGE_SINGLE(NORMAL, logger,"DSAStrategy="<<dsastrategyName);
}

DSAStrategy::~DSAStrategy()
{
}

void
DSAStrategy::setColleagues(RegistryProxyInterface* _registry)
{
	colleagues.registry = _registry;
	assure(colleagues.registry!=NULL,"need colleagues.registry");
	phyModeMapper = colleagues.registry->getPhyModeMapper();
	assure(phyModeMapper!=NULL,"need phyModeMapper");
	wns::service::phy::phymode::PhyModeInterfacePtr phyModePtr =
		phyModeMapper->getHighestPhyMode();
	assure(phyModePtr!=wns::service::phy::phymode::PhyModeInterfacePtr(),"undefined phymode");
	assure(phyModePtr->isValid(),"need valid phymode");
	assure(phyModePtr->dataRateIsValid(),"need phymode with dataRate feature:"<<*phyModePtr);
	highestDataRatePerSubChannel = phyModePtr->getDataRate();
	// UL-slave: I may only use those resources (subchannels,slots) where my userID is written into
	myUserID = colleagues.registry->getMyUserID(); // that's me
	assure(myUserID!=NULL,"cannot get userID");
	MESSAGE_SINGLE(NORMAL, logger, "DSAStrategy::setColleagues("<<myUserID->getName()<<"): bestPhyMode="<<*phyModePtr<<", maxrate/sc="<<highestDataRatePerSubChannel/1e6<<"Mbit/s");
} // setColleagues

// call this before each timeSlot/frame
void
DSAStrategy::initialize(SchedulerStatePtr schedulerState,
			SchedulingMapPtr schedulingMap)
{
	assure(colleagues.registry!=NULL,"need colleagues.registry");
	assure(schedulerState!=SchedulerStatePtr(),"schedulerState must be valid");
	MESSAGE_SINGLE(NORMAL, logger, "DSAStrategy::initialize("<<dsastrategyName<<"): maxrate/sc="<<highestDataRatePerSubChannel/1e6<<"Mbit/s");
} // initialize

simTimeType
DSAStrategy::getCompoundDuration(RequestForResource& request) const
{
	// phyMode may be undefined (most cases). Then it is free to choose (later in AMC,APC).
	wns::service::phy::phymode::PhyModeInterfacePtr phyModePtr = request.phyModePtr;
	simTimeType requestedCompoundDuration = 0;
	assure(highestDataRatePerSubChannel>0.0,"highestDataRatePerSubChannel="<<highestDataRatePerSubChannel);
	if (phyModePtr==wns::service::phy::phymode::PhyModeInterfacePtr())
	{ // empty=undefined phyMode
		requestedCompoundDuration =
		  request.bits / highestDataRatePerSubChannel;
	} else {
		assure(phyModePtr->isValid(),"need valid phymode");
		assure(phyModePtr->dataRateIsValid(),"need phymode with dataRate feature:"<<*phyModePtr);
		double dataRatePerSubChannel = phyModePtr->getDataRate();
		requestedCompoundDuration =
		  request.bits / dataRatePerSubChannel;
	}
	MESSAGE_SINGLE(NORMAL, logger, "getCompoundDuration("<<request.toString()<<"): d="<<requestedCompoundDuration*1e6<<"us");
	return requestedCompoundDuration;
} // getCompoundDuration

int
DSAStrategy::getBeamForSubChannel(int subChannel,
				  int timeSlot,
				  RequestForResource& request,
				  SchedulerStatePtr schedulerState,
				  SchedulingMapPtr schedulingMap) const
{
	assure(subChannel>=0,"need a valid subChannel");
	assure(subChannel<schedulerState->currentState->strategyInput->getFChannels(),"invalid subChannel="<<subChannel);
	assure(timeSlot>=0,"need a valid timeSlot");
	assure(timeSlot<schedulerState->currentState->strategyInput->getNumberOfTimeSlots(),"invalid timeSlot="<<timeSlot);
	int numberOfBeams = schedulerState->currentState->strategyInput->getMaxBeams();
	int getBeam = 0;
	bool ok;
	for ( int beam = 0; beam < numberOfBeams; ++beam )
	{ // only for MIMO. For SISO simply beam=0
		PhysicalResourceBlock& prbDescriptor =
			schedulingMap->subChannels[subChannel].temporalResources[timeSlot]->physicalResources[beam];
		// can be different if "sort" has been applied:
		assure(subChannel==prbDescriptor.subChannelIndex,
		       "subChannel="<<subChannel<<" != subChannelIndex="<<prbDescriptor.subChannelIndex);
		assure(timeSlot==prbDescriptor.timeSlotIndex,
		       "timeSlot="<<timeSlot<<" !=timeSlotIndex="<<prbDescriptor.timeSlotIndex);
		assure(beam==prbDescriptor.beamIndex,
		       "beam="<<beam<<" != beamIndex="<<prbDescriptor.beamIndex);
		// an empty subChannel can always be used:
		if (prbDescriptor.scheduledCompounds.empty())
			{ return beam; }
		// now we are sure that the subChannel is used by at least one packet
		// check if another user is blocking the subChannel:
		if (oneUserOnOneSubChannel)
		{	// checking the first packet is sufficient:
			UserID otherUser = (prbDescriptor.scheduledCompounds.begin())->userID;
			if (otherUser != request.user)
				return beam;
		}
		// check if the PhyMode is already fixed:
		wns::service::phy::phymode::PhyModeInterfacePtr phyModePtr =
			prbDescriptor.phyModePtr;
		RequestForResource requestWithGivenPhyMode = request; // copy
		requestWithGivenPhyMode.phyModePtr = phyModePtr;
		simTimeType compoundDuration = getCompoundDuration(requestWithGivenPhyMode);
		// check if there is enough space (time) left:
		simTimeType remainingTimeOnthisChannel = prbDescriptor.getFreeTime();
		ok = (remainingTimeOnthisChannel - compoundDuration) >= -wns::scheduler::strategy::slotLengthRoundingTolerance;
		if (ok){
			getBeam = beam;
			break;
		}
	}
	return getBeam;
}

bool
DSAStrategy::channelIsUsable(int subChannel,
			     int timeSlot,
			     RequestForResource& request,
			     SchedulerStatePtr schedulerState,
			     SchedulingMapPtr schedulingMap) const
{
	assure(subChannel>=0,"need a valid subChannel");
	assure(subChannel<schedulerState->currentState->strategyInput->getFChannels(),"invalid subChannel="<<subChannel);
	assure(timeSlot>=0,"need a valid timeSlot");
	assure(timeSlot<schedulerState->currentState->strategyInput->getNumberOfTimeSlots(),"invalid timeSlot="<<timeSlot);
	if (!schedulingMap->subChannels[subChannel].subChannelIsUsable) return false; // locked sc?
	//int numberOfTimeSlots = schedulerState->currentState->strategyInput->getNumberOfTimeSlots();
	int numberOfTimeSlots = schedulerState->currentState->strategyInput->getNumberOfTimeSlots();
	int numberOfBeams = schedulerState->currentState->strategyInput->getMaxBeams();
	bool ok;
	//for ( int timeSlot = 0; timeSlot < numberOfTimeSlots; ++timeSlot )
	//{
		for ( int beam = 0; beam < numberOfBeams; ++beam )
		{ // only for MIMO. For SISO simply beam=0
			PhysicalResourceBlock& prbDescriptor =
				schedulingMap->subChannels[subChannel].temporalResources[timeSlot]->physicalResources[beam];
			// can be different if "sort" has been applied:
			assure(subChannel==prbDescriptor.subChannelIndex,
			       "subChannel="<<subChannel<<" != subChannelIndex="<<prbDescriptor.subChannelIndex);
			assure(beam==prbDescriptor.beamIndex,
			       "beam="<<beam<<" != beamIndex="<<prbDescriptor.beamIndex);
			// TODO: copy code from other channelIsUsable() method...
			// an empty subChannel can always be used:
			if (prbDescriptor.scheduledCompounds.empty())
			{ return true; }
			// now we are sure that the subChannel is used by at least one packet
			// check the grouping constraints:
			//if (groupingRequired()) ? <- is this question enough ?
			if (schedulerState->currentState->getGrouping() != GroupingPtr()) // grouping enabled
			{
				// check if request.user fulfils the grouping constraints...
				// get otherUsers on this subchannel
				// if one of them is in the same group as request.user, it's ok to continue
				// if one of them is the same as request.user, this also proves it's ok
				// if one of them in in another group, it's not ok -> return false
				// so we see it's enough to only test one otherUser and not all of them
				// UserID otherUser = (prbDescriptor.scheduledCompounds.begin())->userID;
				// if (!(otherUser is in same group as request.user))
				// { return false; }
			}
			// check if another user is blocking the subChannel:
			if (oneUserOnOneSubChannel)
			{	// checking the first packet is sufficient:
				UserID otherUser = (prbDescriptor.scheduledCompounds.begin())->userID;
				if (otherUser != request.user)
				{
					MESSAGE_SINGLE(NORMAL, logger, "channelIsUsable("<<subChannel<<"): tSlot="<<timeSlot<<", beam="<<beam<<": otherUser="<<otherUser->getName()<<" != request.user="<<request.user->getName());
					return false;
				}
			}
			// check if the PhyMode is already fixed:
			wns::service::phy::phymode::PhyModeInterfacePtr phyModePtr =
				prbDescriptor.phyModePtr;
			RequestForResource requestWithGivenPhyMode = request; // copy
			requestWithGivenPhyMode.phyModePtr = phyModePtr;
			simTimeType compoundDuration = getCompoundDuration(requestWithGivenPhyMode);
			// check if there is enough space (time) left:
			simTimeType remainingTimeOnthisChannel =
				//schedulingMap->subChannels[subChannel].physicalResources[beam].getFreeTime();
				prbDescriptor.getFreeTime();
			ok = (remainingTimeOnthisChannel - compoundDuration) >= -wns::scheduler::strategy::slotLengthRoundingTolerance;
			MESSAGE_SINGLE(NORMAL, logger, "channelIsUsable("<<subChannel<<"): d="<<compoundDuration*1e6<<"us <= "<<remainingTimeOnthisChannel*1e6<<"us remaining: ok="<<ok);
			if (ok) break;
		} // forall beams/streams of this subChannel
	//} // forall timeSlots of this subChannel
	return ok;
} // channelIsUsable

bool
DSAStrategy::channelIsUsable(int subChannel,
			     int timeSlot,
			     int beam,
			     RequestForResource& request,
			     SchedulerStatePtr schedulerState,
			     SchedulingMapPtr schedulingMap) const
{
	assure(subChannel>=0,"need a valid subChannel");
	assure(subChannel<schedulerState->currentState->strategyInput->getFChannels(),"invalid subChannel="<<subChannel);
	assure(timeSlot>=0,"need a valid timeSlot");
	assure(timeSlot<schedulerState->currentState->strategyInput->getNumberOfTimeSlots(),"invalid timeSlot="<<timeSlot);
	assure(beam>=0,"need a valid beam");
	assure(beam<schedulerState->currentState->strategyInput->getMaxBeams(),"invalid beam="<<beam);
	if (!schedulingMap->subChannels[subChannel].subChannelIsUsable) return false; // locked sc?
	// TODO: should we introduce
	// bool allBeamsUsedByOneUserOnly
	PhysicalResourceBlock& prbDescriptor =
		schedulingMap->subChannels[subChannel].temporalResources[timeSlot]->physicalResources[beam];
	// can be different if "sort" has been applied:
	assure(subChannel==prbDescriptor.subChannelIndex,
	       "subChannel="<<subChannel<<" != subChannelIndex="<<prbDescriptor.subChannelIndex);
	assure(beam==prbDescriptor.beamIndex,
	       "beam="<<beam<<" != beamIndex="<<prbDescriptor.beamIndex);
	// check if another user is blocking the subChannel:
	if (oneUserOnOneSubChannel)
	{	// checking the first packet is sufficient:
		//UserID otherUser = prbDescriptor.scheduledCompounds.begin()->userID;
		UserID otherUser = prbDescriptor.getUserID(); // uid of first packet or resource as a whole
		const wns::scheduler::SchedulerSpotType& schedulerSpot = schedulerState->schedulerSpot;
		if (schedulerSpot==wns::scheduler::SchedulerSpot::ULSlave())
		{ // uplink: I am slave. is this resource for me?
			if (otherUser == NULL)
			{
				// an empty subChannel can always be used:
				MESSAGE_SINGLE(NORMAL, logger, "UL-slave: channelIsUsable("<<subChannel<<"."<<timeSlot<<"."<<beam<<"): myUser="<<myUserID->getName()<<", other=NULL => unusableInUL");
				return false;
			}
			if (otherUser != myUserID)
			{
				assure(otherUser!=NULL,"otherUser==NULL");
				MESSAGE_SINGLE(NORMAL, logger, "UL-slave: channelIsUsable("<<subChannel<<"."<<timeSlot<<"."<<beam<<"): myUser="<<myUserID->getName()<<", other="<<otherUser->getName()<<", request="<<request.user->getName()<<": unusable");
				return false;
			}
			// at this point I'm sure that this resource is principially usable by me.
			MESSAGE_SINGLE(NORMAL, logger, "UL-slave: channelIsUsable("<<subChannel<<"."<<timeSlot<<"."<<beam<<"): myUser="<<myUserID->getName()<<", other="<<otherUser->getName()<<" is ok");
		} else { // master scheduler (DL|UL)
			// an empty subChannel can always be used:
			if (prbDescriptor.scheduledCompounds.empty())
			{
				MESSAGE_SINGLE(NORMAL, logger, "channelIsUsable("<<subChannel<<"."<<timeSlot<<"."<<beam<<"): empty channel; can always be used");
				return true;
			}
			// now we are sure that the subChannel is used by at least one packet
			if (otherUser != request.user)
			{
				MESSAGE_SINGLE(NORMAL, logger, "channelIsUsable("<<subChannel<<"."<<timeSlot<<"."<<beam<<"): otherUser="<<otherUser->getName()<<" != request.user="<<request.user->getName());
				return false;
			}
			// at this point I'm sure that this resource is principially usable by me.
		} // slave|master
	} else { // oneUserOnOneSubChannel==false
		// we will have a problem with different txPower and PhyMode on this subChannel
		MESSAGE_SINGLE(NORMAL, logger, "channelIsUsable("<<subChannel<<"."<<timeSlot<<"."<<beam<<"): WARNING: oneUserOnOneSubChannel="<<oneUserOnOneSubChannel<<" is NEW and untested");
		// an empty subChannel can always be used:
		if (prbDescriptor.scheduledCompounds.empty())
		{
			MESSAGE_SINGLE(NORMAL, logger, "channelIsUsable("<<subChannel<<"."<<timeSlot<<"."<<beam<<"): empty channel; can always be used");
			return true;
		}
	}
	// check the grouping constraints:
	//if (groupingRequired()) ? <- is this question enough ?
	if (schedulerState->currentState->groupingIsValid()) // grouping enabled
	{
		// check if request.user fulfils the grouping constraints...
		// get otherUsers on this subchannel
		// if one of them is in the same group as request.user, it's ok to continue
		// if one of them is the same as request.user, this also proves it's ok
		// if one of them in in another group, it's not ok -> return false
		// so we see it's enough to only test one otherUser and not all of them
		// UserID otherUser = (prbDescriptor.scheduledCompounds.begin())->userID;
		// if (!(otherUser is in same group as request.user))
		// { return false; }
	}
	// This is the check if the PhyMode is already fixed:
	wns::service::phy::phymode::PhyModeInterfacePtr phyModePtr =
		prbDescriptor.phyModePtr;
	assure(phyModePtr!=wns::service::phy::phymode::PhyModeInterfacePtr(),"phyModePtr==NULL");
	RequestForResource requestWithGivenPhyMode = request; // copy
	requestWithGivenPhyMode.phyModePtr = phyModePtr;
	simTimeType compoundDuration = getCompoundDuration(requestWithGivenPhyMode);
	// check if there is enough space (time) left:
	simTimeType remainingTimeOnthisChannel =
		//schedulingMap->subChannels[subChannel][beam].getFreeTime();
			prbDescriptor.getFreeTime();
	bool ok = (remainingTimeOnthisChannel - compoundDuration) >= -wns::scheduler::strategy::slotLengthRoundingTolerance;
	MESSAGE_SINGLE(NORMAL, logger, "channelIsUsable("<<subChannel<<"."<<timeSlot<<"."<<beam<<"): d="<<compoundDuration*1e6<<"us <=? "<<remainingTimeOnthisChannel*1e6<<"us remaining: ok="<<ok);
	return ok;
} // channelIsUsable(subChannel,timeSlot,beam)

/*
  Local Variables:
  mode: c++
  fill-column: 80
  c-basic-offset: 8
  c-comment-only-line-offset: 0
  c-tab-always-indent: t
  indent-tabs-mode: t
  tab-width: 8
  End:
*/
