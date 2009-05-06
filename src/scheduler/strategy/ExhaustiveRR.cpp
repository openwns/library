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

#include <WNS/scheduler/strategy/ExhaustiveRR.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>

#include <WNS/scheduler/CallBackInterface.hpp>

#include <vector>
#include <map>
#include <algorithm>
#include <iostream>


using namespace wns::scheduler;
using namespace wns::scheduler::strategy;

STATIC_FACTORY_REGISTER_WITH_CREATOR(ExhaustiveRR,
									 StrategyInterface,
									 "ExhaustiveRR",
									 wns::PyConfigViewCreator);

bool
ExhaustiveRR::everyGroupMemberWasServed(Group group)
{
	// iterate over all users in group.
	// return false if we didn't save the user
	for (Group::const_iterator iter = group.begin();
		 iter != group.end(); ++iter)
		if (usersServedLastRound.find(iter->first) == usersServedLastRound.end())
			return false;
	return true;
}


void
ExhaustiveRR::doStartScheduling(int fChannels, int maxBeams, simTimeType slotLength)
{
	std::vector<double> nextFreeSlot;
	std::set<UserID> usersServedThisFrame;
	usersServedThisFrame.clear();
	UserSet allServableUsers;
	this->resourceUsage = 0.0;

	MESSAGE_BEGIN(NORMAL, logger, m,"ExhaustiveRR::startScheduling called - Tx");
	m << "\n\t Channels:   " << fChannels
	  << "\n\t maxBeams:   " << maxBeams
	  << "\n\t slotLength: " << slotLength;
	MESSAGE_END();

	// reset burst information for map writer
	// (done in Strategy.cpp): clearMap(); // empty bursts

	bool somePDUScheduledForUser;
	bool somePDUScheduledForGrouping;
	bool firstLoop = true;

	nextFreeSlot.clear();
	for (int i = 0; i < fChannels; ++i)
		nextFreeSlot.push_back(0.0);

	do {
		somePDUScheduledForGrouping = false;

		UserSet allUsersInQueue = colleagues.queue->getQueuedUsers();
		UserSet activeUsers     = colleagues.registry->filterReachable(allUsersInQueue);

		//MESSAGE_SINGLE(NORMAL, logger, "allUsersInQueue " << allUsersInQueue.size() );
		//MESSAGE_SINGLE(NORMAL, logger, "activeUsers     " << activeUsers.size() );

		// do not schedule anything if there is nobody active
		if (activeUsers.size() == 0) break;

		Grouping grouping = colleagues.grouper->getTxGrouping(activeUsers, maxBeams);

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

		MESSAGE_SINGLE(NORMAL, logger,"ExhaustiveRR::startScheduling - retrieved grouping from grouper:\n" << grouping.getDebugOutput());
		for (unsigned int group = 0; group < grouping.groups.size(); ++group) {

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

			// all were served, the round is now over, start next round
			if (!unservedUsersLastRound)
			{
				MESSAGE_SINGLE(NORMAL, logger,"All servable users served last round, starting a new round");
				usersServedLastRound.clear();
			}
			else // if not, round is not yet over, check if this group was done;
				 // if yes, skip to the next group
				if (everyGroupMemberWasServed(grouping.groups[group]))
				{
					MESSAGE_SINGLE(NORMAL, logger,"Skipping group "<< group << " because of fairness");
					continue;
				}

			MapInfoEntryPtr currentBurst(new MapInfoEntry());
			// initialize user entry with invalid user ID and empty compound list.
			currentBurst->user = NULL;
			currentBurst->compounds.clear();

			Group currentGroup = grouping.groups[group];

			MESSAGE_SINGLE(NORMAL, logger,"ExhaustiveRR::startScheduling - now scheduling group number " << group);

			if (currentGroup.size() == 0) {
				    MESSAGE_SINGLE(NORMAL, logger,"ExhaustiveRR::startScheduling - Empty group -> skipping");
				continue;
			}

			MESSAGE_BEGIN(VERBOSE, logger, m, "ExhaustiveRR::startScheduling - nextFreeSlots: ");
			for (unsigned int i = 0; i < nextFreeSlot.size(); ++i){
				m << "\t\t\tChannel " << i << ": " << nextFreeSlot[i] << "\n";
			}
			MESSAGE_END();
			// some initialization
			int fSlot = min_element(nextFreeSlot.begin(), nextFreeSlot.end()) - nextFreeSlot.begin();

			MESSAGE_SINGLE(NORMAL, logger,"ExhaustiveRR::startScheduling - chose fSlot=" << fSlot);

			std::vector<UserID> usersInGroup;

			usersInGroup.clear();
			for (Group::iterator iter = currentGroup.begin();
				 iter != currentGroup.end();
				 ++iter) {
				MESSAGE_SINGLE(NORMAL, logger,"added to usersInGroup: " << iter->first);
				usersInGroup.push_back(iter->first);
			}

			std::vector<double> usedInBeam;
			usedInBeam.clear();
			for (unsigned int i = 0; i < usersInGroup.size(); ++i)
				usedInBeam.push_back(nextFreeSlot[fSlot]);

			// get all connections per user in group

			std::map<UserID, ConnectionVector> connectionsPerUser;

			for (unsigned int i = 0; i < usersInGroup.size(); ++i) {
				connectionsPerUser[usersInGroup[i]] =
					colleagues.registry->getConnectionsForUser(usersInGroup[i]);
				MESSAGE_SINGLE(NORMAL, logger,"ExhaustiveRR::startScheduling - user "
							   << colleagues.registry->getNameForUser(usersInGroup[i])
							   << " currently has " << connectionsPerUser[usersInGroup[i]].size() << " active connections");
			}

			// serve all users from a group in parallel beams
			for (unsigned int user = 0; user < currentGroup.size(); ++user) {
				MESSAGE_SINGLE(NORMAL, logger,"ExhaustiveRR::startScheduling - now scheduling user number " << user);

				wns::SmartPtr<const wns::service::phy::phymode::PhyModeInterface> phyModePtr;
				// slave TX scheduling, so we need to use the txPower and phyMode from the masterBurst
				//if (powerControlSlave)
				if (getSchedulerState()->powerControlType==PowerControlULSlave)
				{
					//MapInfoEntryPtr masterBurst = this->getMasterBurst();
					//phyModePtr = masterBurst->phyModePtr;
					phyModePtr = getSchedulerState()->getDefaultPhyMode();
					assure(phyModePtr!=wns::service::phy::phymode::PhyModeInterfacePtr(),"phyModePtr=NULL");
					txPowerPerStream = getTxPower();
					MESSAGE_SINGLE(NORMAL, logger,"UL MAP enabled => we use the txPower="<< txPowerPerStream <<", PhyMode="<<*phyModePtr);
				}
				else
				{
					wns::Ratio sinr(currentGroup[usersInGroup[user]].C / currentGroup[usersInGroup[user]].I);
					phyModePtr = this->getBestPhyMode(sinr);
					//if (!colleagues.registry->getPhyModeMapper()->sinrIsAboveLimit(sinr))
					if (! phyModePtr->isValid() )
					{
						MESSAGE_SINGLE(NORMAL, logger,"RR DL-Scheduler: skipping user because of insufficient PhyMode");
						// nevertheless, mark him as served because we tried our best
						usersServedThisFrame.insert(usersInGroup[user]);
						continue;
					}

					MESSAGE_SINGLE(NORMAL, logger, "PHY mode for: " << colleagues.registry->getNameForUser(usersInGroup[user])
								   << " is : " << *phyModePtr<<" with SINR: "<<sinr << ", Rate: " << phyModePtr->getDataRate() << " bps");
				}
				// get PHYmode for current user
				wns::SmartPtr<const wns::service::phy::phymode::PhyModeInterface> phyMode = phyModePtr;

				do { // try to place as many pdus from the current user as
					 // possible
					somePDUScheduledForUser = false;

					// for current user
					std::vector<ConnectionID> connections;
					connections.clear();
					// get all active connections
					for (ConnectionVector::iterator iter = connectionsPerUser[usersInGroup[user]].begin();
						 iter != connectionsPerUser[usersInGroup[user]].end();
						 ++iter)
						if (colleagues.queue->queueHasPDUs(*iter))
							connections.push_back(*iter);


					/** @brief  Head Of Line Milliseconds - stupid name
						storing how long it would
						take to transmit each of the Head of Line PDUs for this user.
					*/
					std::map<int, double> HoLms;
					for (unsigned int i = 0; i < connections.size(); ++i)
					{
						HoLms[i] = colleagues.queue->getHeadOfLinePDUbits(connections[i]) /	phyMode->getDataRate();
						MESSAGE_SINGLE(NORMAL, logger,"bits=" <<colleagues.queue->getHeadOfLinePDUbits(connections[i]) << " dataRate=" <<	phyMode->getDataRate() << ", HoLms=" << HoLms[i] << ", BitsperSymbol=" << phyMode->getBitsPerSymbol() << ", Modulation=" << phyMode->getString());
					}

					for (unsigned int cid = 0; cid < connections.size(); ++cid){
						MESSAGE_SINGLE(NORMAL, logger,"ExhaustiveRR::startScheduling - now scheduling connection number " << connections[cid]);
						MESSAGE_SINGLE(NORMAL, logger,"slotLength=" << slotLength << ", used=" << usedInBeam[user] << ", HoLms=" << HoLms[cid]);
//sdr					if (HoLms[cid] < slotLength - usedInBeam[user]) { // user fits in
						if (HoLms[cid] <= slotLength - usedInBeam[user] + slotLengthRoundingTolerance) { // user fits in
							bool firstPDU = false;

							// update the burst info for the map
							// if still the same user, update current burst end
							if (usersInGroup[user] == currentBurst->user)
							{
							  currentBurst->end = usedInBeam[user]+HoLms[cid];
							  //MESSAGE_SINGLE(NORMAL, logger,"ExhaustiveRR: a)  startTime " << currentBurst->start << ", stopTime = " << currentBurst->end);
							}
							else // start new burst
							{
								currentBurst = MapInfoEntryPtr(new MapInfoEntry());
								currentBurst->start = currentBurst->end = usedInBeam[user];
								currentBurst->end = usedInBeam[user]+HoLms[cid];
								currentBurst->user = usersInGroup[user];
								currentBurst->subBand = fSlot;
								currentBurst->phyModePtr = phyMode;
								currentBurst->txPower = txPowerPerStream;
								bursts_push_back(currentBurst);
								firstPDU = true;
								MESSAGE_SINGLE(NORMAL, logger,"ExhaustiveRR: b)  startTime " << currentBurst->start << ", stopTime = " << currentBurst->end);
							}

							MESSAGE_SINGLE(NORMAL, logger,"ExhaustiveRR: a)  startTime " << currentBurst->start << ", stopTime = " << currentBurst->end);
							MESSAGE_SINGLE(NORMAL, logger,"ExhaustiveRR: startTime " << usedInBeam[user] << ", stopTime = " << usedInBeam[user]+HoLms[cid]);

							MESSAGE_SINGLE(NORMAL, logger,"ExhaustiveRR: txPower = " << txPowerPerStream);

							wns::ldk::CompoundPtr pdu = colleagues.queue->getHeadOfLinePDU(connections[cid]);
							bursts_push_back_compound(pdu);
							compoundReady(fSlot,
										  usedInBeam[user],
										  usedInBeam[user]+HoLms[cid],
										  usersInGroup[user],
										  pdu,
										  user,
										  grouping.patterns[usersInGroup[user]],
										  currentBurst,
										  *phyMode,
										  txPowerPerStream,
										  currentGroup[usersInGroup[user]]);

							///\todo overhead calculation
							usedInBeam[user] += HoLms[cid];
							somePDUScheduledForUser = true;
							somePDUScheduledForGrouping = true;

							///\todo Find a better criterion here
							// only mark user as served if he got a significant
							// share of the frame, i.e., more than 2%
							// like this, mgmt-only bursts are not counted
							if ((currentBurst->end - currentBurst->start) / slotLength > 0.02)
								usersServedThisFrame.insert(usersInGroup[user]);
						}
						else
						{
							MESSAGE_BEGIN(NORMAL, logger, m, "ExhaustiveRR::startScheduling: ");
							m << "Skipping User "<< colleagues.registry->getNameForUser(usersInGroup[user])
							  << ". Its next PDU (size=" << HoLms[cid]
							  << "s) does not fit the slot (remaining size=" << slotLength - usedInBeam[user]
							  << "s).";
							MESSAGE_END();
						}

					}
				} while (somePDUScheduledForUser);

				///\todo At this point, the ExhaustiveRR should take the
				///symbolDuration into account and advance the starting time for
				///the next user to a multiple of an OFDM symbol.

				// Now try the other users in the group
			} // for user
			// no overlapping transmissions on this channel in the future
			assure(usedInBeam.begin() != usedInBeam.end(), "Empty interval");
			nextFreeSlot[fSlot] = *max_element(usedInBeam.begin(), usedInBeam.end());
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
				MESSAGE_SINGLE(NORMAL, logger,"User " << colleagues.registry->getNameForUser(*iter) << " was served this frame");
				usersServedLastRound.insert(*iter);
			}
			else {
				MESSAGE_SINGLE(NORMAL, logger,"User " << colleagues.registry->getNameForUser(*iter) << " was not yet served this frame");
				// if not served this round nor before, he is still unserved
				if (usersServedLastRound.find(*iter) == usersServedLastRound.end()) {
					unservedUsersLastRound = true;
					MESSAGE_SINGLE(NORMAL, logger,"and was also not yet served this round");
				}
			}
		}
		// now get a new set of active users, new grouping etc and start again
	} while (somePDUScheduledForGrouping);
	MESSAGE_SINGLE(NORMAL, logger,"unservedUsersLastFrame: " << unservedUsersLastRound);

	double usedPercentage = 0.0;
	for (int ii = 0; ii < fChannels; ++ii)
		usedPercentage += nextFreeSlot.at(ii) / slotLength;
	this->resourceUsage = usedPercentage / fChannels;
	MESSAGE_SINGLE(NORMAL, logger,"Resources allocated in this Frame ("<<fChannels<<" subchannels): " << 100.0*this->resourceUsage << "%");
} // doStartScheduling
