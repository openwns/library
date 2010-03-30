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

#include <WNS/scheduler/strategy/CQIEnabledExhaustiveRR.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>

#include <WNS/scheduler/CallBackInterface.hpp>

#include <vector>
#include <map>
#include <algorithm>
#include <iostream>


using namespace wns::scheduler;
using namespace wns::scheduler::strategy;

STATIC_FACTORY_REGISTER_WITH_CREATOR(CQIEnabledExhaustiveRR,
				     StrategyInterface,
				     "CQIEnabledExhaustiveRR",
				     wns::PyConfigViewCreator);


void
CQIEnabledExhaustiveRR::onColleaguesKnown()
{
	Strategy::onColleaguesKnown();
	phyModeMapper = colleagues.registry->getPhyModeMapper();
	assure(phyModeMapper != NULL, "invalid phyModeMapper");

	PowerCapabilities powerLimit = colleagues.registry->getPowerCapabilities();
	maxPowerPerSubChannel = powerLimit.maxPerSubband;
	maxSummedPowerOnAllChannels = powerLimit.maxOverall;
	nominalPowerPerSubChannel = powerLimit.nominalPerSubband;

	MESSAGE_SINGLE(NORMAL, logger,"The maxPowerPerSubChannel: "<<maxPowerPerSubChannel
				   <<" The maxSummedPowerOnAllChannels: "<<maxSummedPowerOnAllChannels
				   <<" The nominalPowerPerSubChannel: "<<nominalPowerPerSubChannel);

	if (useNominalTxPower)
	{
		MESSAGE_SINGLE(NORMAL, logger, "we are told to always use nominal txpower, so no adaptive power control here");
	} else {
		MESSAGE_SINGLE(NORMAL, logger, "we perform adaptive power control here");
	}
}

wns::Power
CQIEnabledExhaustiveRR::getRemainedTxPower(int /*channel*/, double beginTime, wns::Power maxSummedPowerOnAllChannels, usedTxPowerOnAllChannels& usedTxPower)
{
	wns::Power remainedPower = maxSummedPowerOnAllChannels;
	for(unsigned int i = 0; i < usedTxPower.size(); i++ )
	{
		//we have no PDU allocated on this channel, just skip it.
		if(usedTxPower[i].size() == 0)
		{
			continue;
		}

		//on each channel there are may be several PDU, all the PDU, which still
		//not finish, will affect the Tx Power allocation of this PDU. Because we still
		//don't know how long will this PDU last,(to know it we need to know the
		//PhyMode, but to know the PhyMode we need to know the txPower, but now
		//the txPowe can not be decided yet, we just use the safest way: we
		//record the max txPower used by the rest PDU)

		wns::Power maxTxPowerOnThisChannel;
		for(unsigned int j = 0; j < usedTxPower[i].size(); j++)
		{
			if( usedTxPower[i][j].endTime > beginTime && maxTxPowerOnThisChannel < usedTxPower[i][j].txPower  )
			{
				maxTxPowerOnThisChannel = usedTxPower[i][j].txPower;
			}
		}
		remainedPower -= maxTxPowerOnThisChannel;
	}
	return remainedPower;
}

bool
CQIEnabledExhaustiveRR::everyGroupMemberWasServed(Group group)
{
	// iterate over all users in group.
	// return false if we didn't save the user
	for (Group::const_iterator iter = group.begin();
		 iter != group.end(); ++iter)
		if (usersServedLastRound.find(iter->first) == usersServedLastRound.end())
			return false;
	return true;
}


ChannelQualityOnOneSubChannel
CQIEnabledExhaustiveRR::getBestChannel(ChannelQualitiesOnAllSubBandsPtr channelQualities,std::vector<double>& nextFreeSlot, double slotLength, usedTxPowerOnAllChannels& usedTxPower)
{
	bestChannel = 0;
	double bestCapacity = 0.0;

	// here we assume that we get channel qualities on all the channels
	for (unsigned int i = 0; i < channelQualities->size(); i++ )
	{
		uint32_t currentChannel = i;
		if (currentChannel < nextFreeSlot.size())
		{
			double remainedTimeOnthisChannel = slotLength - nextFreeSlot[currentChannel];
			double capacityOfThisChannel = 0.0;

			if (useNominalTxPower)
			{
			  wns::Ratio maxSINR = nominalPowerPerSubChannel/((*channelQualities)[i].interference * (*channelQualities)[i].pathloss.get_factor());
				wns::SmartPtr<const wns::service::phy::phymode::PhyModeInterface> bestPhyMode = phyModeMapper->getBestPhyMode(maxSINR);
				capacityOfThisChannel = bestPhyMode->getDataRate() * remainedTimeOnthisChannel;
			}
			else
			{
				wns::Power remainedTxPowerOnAllChannels = getRemainedTxPower(currentChannel, nextFreeSlot[currentChannel],maxSummedPowerOnAllChannels,usedTxPower);
				wns::Power maxTxPower =
					(maxPowerPerSubChannel > remainedTxPowerOnAllChannels) ?
					remainedTxPowerOnAllChannels : maxPowerPerSubChannel;
				wns::Ratio maxSINR = maxTxPower/((*channelQualities)[i].interference * (*channelQualities)[i].pathloss.get_factor());
				wns::SmartPtr<const wns::service::phy::phymode::PhyModeInterface> bestPhyMode = phyModeMapper->getBestPhyMode(maxSINR);
				capacityOfThisChannel = bestPhyMode->getDataRate() * remainedTimeOnthisChannel;
			}

			if (capacityOfThisChannel > bestCapacity)
			{
				bestChannel = i;
				bestCapacity = capacityOfThisChannel;
			}
		}
	}
	return (*channelQualities)[bestChannel];
}


void
CQIEnabledExhaustiveRR::doStartScheduling(int fChannels, int maxSpatialLayers, simTimeType slotLength)
{
	std::vector<double> nextFreeSlot;
	std::set<UserID> usersServedThisFrame;
	usersServedThisFrame.clear();
	UserSet allServableUsers;

	MESSAGE_BEGIN(NORMAL, logger, m,"CQIExhaustiveRR::startScheduling called - Tx");
	m << "\n\t Channels:   " << fChannels
	  << "\n\t maxSpatialLayers:   " << maxSpatialLayers
	  << "\n\t slotLength: " << slotLength;
	MESSAGE_END();

	// reset burst information for map writer
	// (done in Strategy.cpp): clearMap(); // empty bursts

	bool somePDUScheduledForUser;
	bool somePDUScheduledForGrouping;
	bool firstLoop = true;
	double roundingTolerance = 1.0E-12;

	nextFreeSlot.clear();
	for (int i = 0; i < fChannels; ++i)
		nextFreeSlot.push_back(0.0);

	usedTxPowerOnAllChannels usedTxPower(fChannels);

	std::map<UserID, ChannelQualitiesOnAllSubBandsPtr> channelQualitiesOfUsers;

	do {
		somePDUScheduledForGrouping = false;

		UserSet allUsersInQueue = colleagues.queue->getQueuedUsers();
		UserSet activeUsers = colleagues.registry->filterReachable(allUsersInQueue);

		// do not schedule anything if there is nobody active
		if (activeUsers.size() == 0)
			break;

		Grouping grouping = colleagues.grouper->getTxGrouping(activeUsers, maxSpatialLayers);

		// extract the users that are actually servable; we need this to assure
		// some fairness; every user gets a pattern so use the map of patterns
		// to get users in grouping; do this only in the first loop
		if (firstLoop)
		{
			allServableUsers.clear();
			for (std::map<UserID, wns::service::phy::ofdma::PatternPtr>::const_iterator iter = grouping.patterns.begin();
				 iter != grouping.patterns.end(); ++iter)
				allServableUsers.insert(iter->first);

			MESSAGE_SINGLE(NORMAL, logger,"I have " << allServableUsers.size() << " servable users");
			firstLoop = false;
		}

		wns::Power txPowerPerStream;

		for (unsigned int group = 0; group < grouping.groups.size(); ++group) {

			// adapt TxPower of each stream of the group
			txPowerPerStream = nominalPowerPerSubChannel;
			txPowerPerStream += grouping.shareOfPowerPerStreams(group);
			if(isEirpLimited()){
				txPowerPerStream += grouping.eirpReductionOfPower(group);
			}


			// all were served, the round is now over, start next round
			if (!unservedUsersLastRound)
			{
				usersServedLastRound.clear();
			}
			else // if not, round is not yet over, check if this group was done;
				 // if yes, skip to the next group
				if (everyGroupMemberWasServed(grouping.groups[group]))
				{
					continue;
				}

			MapInfoEntryPtr currentBurst(new MapInfoEntry());
			// initialize user entry with invalid user ID and empty compound list.
			currentBurst->user = NULL;
			currentBurst->compounds.clear();

			Group currentGroup = grouping.groups[group];

			if (currentGroup.size() == 0) {
				continue;
			}

			assure(currentGroup.size() == 1, "Now it only works for 1 user group!!!");

			UserID userInGroup = currentGroup.begin()->first;

			// to avoid calling registry->getChannelsQualities4UserOnDownlink() several times.
			ChannelQualitiesOnAllSubBandsPtr channelQualities;
			if (channelQualitiesOfUsers.count(userInGroup) == 0)
			{ // first time call
				channelQualities = colleagues.registry->getChannelQualities4UserOnDownlink(userInGroup, 0/*frameNr*/);
				if (channelQualities != ChannelQualitiesOnAllSubBandsPtr()) {
				  // Warning: This works on the original datastructure and has O(N*log(N)^2) !:
				  std::stable_sort(channelQualities->begin(), channelQualities->end(), betterChannelQuality());
				  channelQualitiesOfUsers[userInGroup] = channelQualities;
				}
			}
			else
			{
				channelQualities = channelQualitiesOfUsers[userInGroup];
			}
			// remark: ^ copying this vector is inefficient. Find a better way.

			wns::Power requiredTxPower;
			std::pair<wns::SmartPtr<const wns::service::phy::phymode::PhyModeInterface>, DataRate> mode;
			int fSlot = 0;
			double usedTimeOnTheChannel = 0.0;

			// sometime we can not get the channel qualities,
			// like: due to some specifical design of WNS, BS may need to do scheduling for RN without sending or receiveing any PDU from it
			if((channelQualities!=ChannelQualitiesOnAllSubBandsPtr())
			   && (channelQualities->size() > 0)) // if CQI usage is enabled
			{ // with CQI information:
				MESSAGE_SINGLE(NORMAL, logger,"We get channel qualities for "<<colleagues.registry->getNameForUser(userInGroup)<<", The Scheduling procedure in this round is: ");
				// some initialization
				ChannelQualityOnOneSubChannel channelQualityOnBestChannel = getBestChannel(channelQualities, nextFreeSlot,slotLength, usedTxPower);
				fSlot = bestChannel;
				usedTimeOnTheChannel = nextFreeSlot[fSlot];

				if (useNominalTxPower)
				{ // no power control
					requiredTxPower = nominalPowerPerSubChannel;
					wns::Ratio rxSINR = requiredTxPower/(channelQualityOnBestChannel.interference * channelQualityOnBestChannel.pathloss.get_factor());
					mode.first = (phyModeMapper->getBestPhyMode(rxSINR));
					mode.second = mode.first->getDataRate();
					MESSAGE_SINGLE(NORMAL, logger, "rx-SINR with nominalPower: "
								   <<nominalPowerPerSubChannel<<" is: "<<rxSINR
								   <<"with PhyMode"<<*(mode.first));
				}
				else
				{ // perform power control here...
					wns::Power remainedTxPowerOnAllChannels = getRemainedTxPower(fSlot, nextFreeSlot[fSlot],maxSummedPowerOnAllChannels,usedTxPower);
					wns::Power maxTxPower = (maxPowerPerSubChannel > remainedTxPowerOnAllChannels ? remainedTxPowerOnAllChannels : maxPowerPerSubChannel );
					MESSAGE_SINGLE(NORMAL, logger, "maxTxPower can be used on the channel "<<fSlot<<" is: "<<maxTxPower);

					wns::Ratio maxSINR = maxTxPower/(channelQualityOnBestChannel.interference * channelQualityOnBestChannel.pathloss.get_factor());
					MESSAGE_SINGLE(NORMAL, logger, "max-rx-SINR with maxTxPower "<<maxTxPower<<" is: "<<maxSINR);
					mode.first = (phyModeMapper->getBestPhyMode(maxSINR));

					//we always try to use the minimal tx power for ceratin phymode to save power
					wns::Ratio minSINR = phyModeMapper->getMinSINRRatio(mode.first);
					MESSAGE_SINGLE(NORMAL, logger, "minSINR required for schedulered PhyMode "<<*(mode.first)<<" is: "<<minSINR);
					requiredTxPower = wns::Power::from_mW(minSINR.get_factor() * channelQualityOnBestChannel.pathloss.get_factor() * channelQualityOnBestChannel.interference.get_mW() ) ;
					MESSAGE_SINGLE(NORMAL, logger, "requiredTxPower with minSINR is: "<<requiredTxPower);

					// get PHYmode for current user
					mode.second = mode.first->getDataRate();
				}
			} // with CQI information
			else
			{ // without CQI information:
				MESSAGE_SINGLE(NORMAL, logger,"No channel qualities for "<<colleagues.registry->getNameForUser(userInGroup)<<", The Scheduling procedure in this round is: ");

				fSlot = min_element(nextFreeSlot.begin(), nextFreeSlot.end()) - nextFreeSlot.begin();
				usedTimeOnTheChannel = nextFreeSlot[fSlot];

				// we try to randomly distribute the channels to user, to get better channel quality
				if(useRandomChannelAtBeginning)
				{
					MESSAGE_SINGLE(NORMAL, logger,"We choose channel randomly for this user, fSlot before is : "<<fSlot);
					int numOfSameChannel = count(nextFreeSlot.begin(), nextFreeSlot.end(), usedTimeOnTheChannel );

					// there are more than 1 channel with this minimal used time, so it is possible to randomly use one
					// channel among them.
					if(numOfSameChannel > 1)
					{
						MESSAGE_SINGLE(NORMAL, logger,"numOfSameChannel is: "<<numOfSameChannel);
						int randomNumber = rand();
						//whichOneToChoose is from 1 to numOfSameChannel
						int whichOneToChoose =1+ randomNumber%numOfSameChannel;

						MESSAGE_SINGLE(NORMAL, logger,"whichOneToChoose is: "<<whichOneToChoose);

						int indexOfchannel = 0;
						for(unsigned int i = 0; i < nextFreeSlot.size(); i++)
						{
							if(nextFreeSlot[i] == usedTimeOnTheChannel)
							{
								indexOfchannel++;
							}
							if(indexOfchannel == whichOneToChoose)
							{
								fSlot = i;
								break;
							}
						}
					}

					MESSAGE_SINGLE(NORMAL, logger,"channel randomly chosen, fSlot after is : "<<fSlot);
				}
				// find how many channels have this minimal used time
				if(useNominalTxPower)
				{
					requiredTxPower = nominalPowerPerSubChannel;
				}
				else
				{ // perform power control here...
					wns::Power remainedTxPowerOnAllChannels = getRemainedTxPower(fSlot, nextFreeSlot[fSlot],maxSummedPowerOnAllChannels,usedTxPower);
					requiredTxPower = (txPowerPerStream > remainedTxPowerOnAllChannels ? remainedTxPowerOnAllChannels : txPowerPerStream );
				}

				wns::Ratio sinr(currentGroup[userInGroup].C / currentGroup[userInGroup].I);
				mode.first  = (phyModeMapper->getBestPhyMode(sinr));
				mode.second = mode.first->getDataRate();

				MESSAGE_SINGLE(NORMAL, logger,"channel: "<<fSlot<<" txPower: "<<requiredTxPower<<" PhyMode: "<<*(mode.first));
			} // with|without CQI information



			if ( mode.first->isValid() == false || requiredTxPower == wns::Power() )
			{
				MESSAGE_SINGLE(NORMAL, logger,"RR DL-Scheduler: skipping user because of insufficient PhyMode");

				// nevertheless, mark him as served because we tried our best
				usersServedThisFrame.insert(userInGroup);
				continue;
			}

			MESSAGE_SINGLE(NORMAL, logger,"The Schedulering result for "<<colleagues.registry->getNameForUser(userInGroup));

			do { // try to place as many pdus from the current user as
				 // possible
				somePDUScheduledForUser = false;

				// for current user
				std::vector<ConnectionID> connections;
				connections.clear();
				// get all active connections
				ConnectionVector connectionsOfUser = colleagues.registry->getConnectionsForUser(userInGroup);
				for (ConnectionVector::iterator iter = connectionsOfUser.begin();
					 iter != connectionsOfUser.end();
					 ++iter)
					if (colleagues.queue->queueHasPDUs(*iter))
						connections.push_back(*iter);

				/** @brief  Head Of Line Milliseconds - stupid name
					storing how long it would
					take to transmit each of the Head of Line PDUs for this user.
				*/
				std::map<int, double> HoLms;
				for (unsigned int i = 0; i < connections.size(); ++i)
					HoLms[i] = colleagues.queue->getHeadOfLinePDUbits(connections[i]) /
						mode.second;

				for (unsigned int cid = 0; cid < connections.size(); ++cid) {

					if (HoLms[cid] <= slotLength - usedTimeOnTheChannel + roundingTolerance) { // user fits in

						bool firstPDU = false;

						// update the burst info for the map
						// if still the same user, update current burst end
						if (userInGroup == currentBurst->user)
							currentBurst->end = usedTimeOnTheChannel+HoLms[cid];
						else // start new burst
						{
							currentBurst = MapInfoEntryPtr(new MapInfoEntry());
							currentBurst->start = currentBurst->end = usedTimeOnTheChannel;
							currentBurst->end = usedTimeOnTheChannel+HoLms[cid];
							currentBurst->user = userInGroup;
							currentBurst->subBand = fSlot;
							currentBurst->phyModePtr = mode.first;
							currentBurst->txPower = txPowerPerStream;
							bursts_push_back(currentBurst);
							firstPDU = true;
						}

						wns::ldk::CompoundPtr pdu = colleagues.queue->getHeadOfLinePDU(connections[cid]);
						//bursts.back()->compounds.push_back(pdu);
						bursts_push_back_compound(pdu);
						compoundReady(fSlot,
							      usedTimeOnTheChannel,
							      usedTimeOnTheChannel+HoLms[cid],
							      userInGroup,
							      pdu,
							      0,  //0 spatialLayers
							      grouping.patterns[userInGroup],
							      currentBurst,
							      *(mode.first),
							      requiredTxPower,
							      currentGroup[userInGroup]);

						///\todo overhead calculation
						usedTimeOnTheChannel += HoLms[cid];
						somePDUScheduledForUser = true;
						somePDUScheduledForGrouping = true;

						TxPower4PDU txPowerUsage(usedTimeOnTheChannel,usedTimeOnTheChannel+HoLms[cid],requiredTxPower);
						usedTxPower[fSlot].push_back(txPowerUsage);


						MESSAGE_SINGLE(NORMAL, logger,"Channel: "<<fSlot<<" PhyMode: "<< *(mode.first)<<" txPower: "<<requiredTxPower<<" startT: "<<usedTimeOnTheChannel<<" endT: "<<usedTimeOnTheChannel+HoLms[cid]);

						///\todo Find a better criterion here
						// only mark user as served if he got a significant
						// share of the frame, i.e., more than 2%
						// like this, mgmt-only bursts are not counted
						if ((currentBurst->end - currentBurst->start) / slotLength > 0.02)
							usersServedThisFrame.insert(userInGroup);
					}

				}
			} while (somePDUScheduledForUser);

			///\todo At this point, the ExhaustiveRR should take the
			///symbolDuration into account and advance the starting time for
			///the next user to a multiple of an OFDM symbol.

			nextFreeSlot[fSlot] = usedTimeOnTheChannel;
			// now schedule the other groups
		} // for group

		// keep in mind if there were unserved users and who they were
		unservedUsersLastRound = false;

		// mark all users served this frame as served in last round
		for (UserSet::iterator iter = allServableUsers.begin();
			 iter != allServableUsers.end(); ++iter) {
			// this user was just served, add him to those served last round
			if (usersServedThisFrame.find(*iter) != usersServedThisFrame.end())
			{
				usersServedLastRound.insert(*iter);
			}
			else {
				// if not served this round nor before, he is still unserved
				if (usersServedLastRound.find(*iter) == usersServedLastRound.end()) {
					unservedUsersLastRound = true;
				}
			}
		}
		// now get a new set of active users, new grouping etc and start again
	} while (somePDUScheduledForGrouping);
}
