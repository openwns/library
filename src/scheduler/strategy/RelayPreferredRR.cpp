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

#include <WNS/scheduler/strategy/RelayPreferredRR.hpp>
#include <WNS/scheduler/CallBackInterface.hpp>

#include <iostream>

using namespace wns::scheduler;
using namespace wns::scheduler::strategy;


STATIC_FACTORY_REGISTER_WITH_CREATOR(
   RelayPreferredRR,
   StrategyInterface,
   "RelayPreferredRR",
   wns::PyConfigViewCreator);


#if defined(WNS_NDEBUG) && defined(WNS_NO_LOGGING)
void RelayPreferredRR::doStartScheduling(int /*fChannels*/, int maxSpatialLayers, simTimeType slotLength)
#else
void RelayPreferredRR::doStartScheduling(int fChannels, int maxSpatialLayers, simTimeType slotLength)
#endif
{

	MESSAGE_SINGLE(NORMAL, logger,"RelayPreferredRoundRobin::startScheduling called - Rx"
				   << "\n\t Channels:   " << fChannels
				   << "\n\t maxSpatialLayers:   " << maxSpatialLayers
				   << "\n\t slotLength: " << slotLength);

	assure(fChannels == 1, "this scheduler does only work with 1 fChannel");

	//UserSet activeUsers = colleagues.registry->filterReachable(colleagues.queue->getQueuedUsers());
	UserSet	allUsers    = colleagues.queue->getQueuedUsers();
	UserSet activeUsers = colleagues.registry->filterReachable(allUsers);
	if (activeUsers.size() == 0) return;

	// We are going to schedule a burst for every user

	Grouping grouping = colleagues.grouper->getRxGrouping(activeUsers, maxSpatialLayers);

	MESSAGE_SINGLE(NORMAL, logger,"RoundRobin::startScheduling Rx - retrieved grouping from grouper:\n" << grouping.getDebugOutput());

	// We give every group the same amount of time in one of the subBands.
	// Of course, the maximal burstlength is the slotlength, no multiple bursts
	// are scheduled for a group

	// We assume that one of the active groups is the relay group.
	int numMiniSlots = grouping.groups.size() - 1 + nRemoteStations_;

	double regularBurstLength = slotLength / double(numMiniSlots);
	double relayBurstLength = regularBurstLength * nRemoteStations_;

	int currentSubBand = 0;
	//	int posInSubBand = 0;
	// (done in Strategy.cpp): clearMap(); // empty bursts

	wns::Power txPowerPerStream;

	simTimeType lastBurstEnd = 0.0;
	// every group is assigned exactly one burst-slot
	for (unsigned int group = 0; group < grouping.groups.size(); ++group)
	{
		//assure(currentSubBand < fChannels, "Big confusion concerning total subBands and bursts");

		Group currentGroup = grouping.groups[group];

		// adapt TxPower of each stream of the group
		txPowerPerStream = getTxPower();
		if(!isEirpLimited()){
			txPowerPerStream += grouping.shareOfPowerPerStreams(group);
			MESSAGE_SINGLE(NORMAL, logger, "share of power per concurrent stream: "
				       << grouping.shareOfPowerPerStreams(group).get_factor());
		} else {
			txPowerPerStream += grouping.eirpReductionOfPower(group);
			MESSAGE_SINGLE(NORMAL, logger, "Tx power reduction due to EIRP: "
				       << grouping.eirpReductionOfPower(group).get_factor());
		}

		simTimeType burstStart = lastBurstEnd;
		simTimeType burstEnd;

		if( isGroupRelayGroup( currentGroup) )
			burstEnd = burstStart + relayBurstLength - slotLengthRoundingTolerance;
		else
			burstEnd = burstStart + regularBurstLength - slotLengthRoundingTolerance;

		lastBurstEnd = burstEnd + slotLengthRoundingTolerance;

		// for every user we provide one MapInfoEntry and tell the parent to set
		// the timingcommand for the dummy pdu so that the patterns get set
		int spatialLayer = 0;
		for (Group::iterator iter = currentGroup.begin();
		     iter != currentGroup.end(); ++iter) {
			UserID user = iter->first;

			MapInfoEntryPtr currentBurst(new MapInfoEntry());

			wns::Ratio sinr(currentGroup[user].C / currentGroup[user].I);
			wns::SmartPtr<const wns::service::phy::phymode::PhyModeInterface> phyMode =
				this->getBestPhyMode(sinr);

			currentBurst->start = burstStart;
			currentBurst->end = burstEnd;
			currentBurst->user = user;
			currentBurst->subBand = currentSubBand;
			currentBurst->phyModePtr = phyMode;
			currentBurst->estimatedCandI = currentGroup[user];

			///\todo FIXME: provider a cleaner handling for the situation that
			///one of the users in the grouping does not have a sufficient SINR
			///estimate for transmission
			//if (int(currentBurst->phyMode) != noPhyMode)
			if (phyMode->isValid())
			{
				bursts_push_back(currentBurst);

				wns::ldk::CompoundPtr pdu;
				// we can delete all other BWReqs for the user for this
				// round
				colleagues.queue->resetQueues(user);

				compoundReady(currentSubBand,
					      burstStart,
					      burstEnd,
					      user,
					      pdu,
					      spatialLayer,
					      grouping.patterns[user],
					      currentBurst,
					      *phyMode,
					      txPowerPerStream,
					      currentGroup[user]);
				spatialLayer++;
			} else {
				MESSAGE_SINGLE(NORMAL, logger,"RR UL scheduler ignoring user with bad PhyMode or too low SINR");
			}
		}

		// calculate the location of the next Burst
//		posInSubBand++;
//		if (posInSubBand == burstsPerSubBand)
//		{
//			posInSubBand = 0;
//			currentSubBand++;
//		}
	}
}

bool RelayPreferredRR::isGroupRelayGroup( const Group& group )
{
	bool isRelayGroup = false;
	for ( Group::const_iterator iter = group.begin();
		  iter != group.end();
		  ++iter )
	{
		if( colleagues.registry->getStationType( iter->first ) == wns::service::dll::StationTypes::FRS() )
			isRelayGroup = true;
		else
			assure( !isRelayGroup, "there is a station in a relay group that is not a relay station");
	}
	return isRelayGroup;
}



