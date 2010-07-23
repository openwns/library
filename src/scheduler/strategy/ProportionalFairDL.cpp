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

#include <WNS/scheduler/strategy/ProportionalFairDL.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>
#include <WNS/scheduler/CallBackInterface.hpp>

#include <vector>
#include <map>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <queue>
#include <math.h>

using namespace wns::scheduler;
using namespace wns::scheduler::strategy;


STATIC_FACTORY_REGISTER_WITH_CREATOR(ProportionalFairDL,
				     StrategyInterface,
				     "ProportionalFairDL",
				     wns::PyConfigViewCreator);


ProportionalFairDL::ProportionalFairDL(const wns::pyconfig::View& config)
	: ProportionalFairBase(config),
	  oneBurstSucceeded(false),
	  allUsers(),
	  resourceUsage(0.0)
{
	allUsers.clear();
}

simTimeType
ProportionalFairDL::scheduleOneBurst(simTimeType burstStart, simTimeType burstLength, Group group, int subBand,
				   std::map<UserID, wns::service::phy::ofdma::PatternPtr> patterns,
				   bool allowForever, wns::Power txPowerPerStream)
{
	std::map<UserID, wns::SmartPtr<const wns::service::phy::phymode::PhyModeInterface> > userPhyModes;
	std::map<UserID, MapInfoEntryPtr> userBursts;
	std::map<UserID, simTimeType> timeMarkers;
	std::map<UserID, bool> finished;
	std::map<UserID, int> spatialLayerId;
	int spatialLayer = 0;
	simTimeType burstLengthMax = burstLength;
	simTimeType currentBurstEnd = 0.0;

	MESSAGE_SINGLE(NORMAL, logger, "scheduleOneBurst(subBand="<<subBand<<","<<printGroup(group)<<")");

	// get phy modes for group members and initialize some data structures
	for (Group::const_iterator iter = group.begin();
		 iter != group.end(); ++iter)
	{
		UserID user = iter->first;

		wns::Ratio sinr(iter->second.C / iter->second.I);
		wns::SmartPtr<const wns::service::phy::phymode::PhyModeInterface> phyMode =
			this->getBestPhyMode(sinr);
		userPhyModes[user] = phyMode;

		// initialize the burst info for the map writer
		MapInfoEntryPtr currentBurst(new MapInfoEntry());
		currentBurst->start = burstStart;
		currentBurst->user = user;
		currentBurst->subBand = subBand;
		currentBurst->phyModePtr = phyMode;
		currentBurst->txPower = txPowerPerStream;
		userBursts[user] = currentBurst;

		// stores the time when the next PDU could be scheduled relative to
		// burst start
		timeMarkers[user] = 0.0;

		// holds a flag if the user cannot fit the next pdu into the burst
		finished[user] = false;

		// this is just for plotting purposes:
		spatialLayerId[user] = spatialLayer;
		spatialLayer++;
	}

	do // while not everybody finished or some user without data
	{
		// get the unfinished user with the next free opportunity
		UserID earliest = getNextUnfinished(timeMarkers, finished);

                // be safe
		if (!earliest.isValid()){
			MESSAGE_SINGLE(NORMAL, logger, "all users finished");
			break;
		}

		// should deliver the cid for the user's queue that is to be served next
		ConnectionID cid = getNextConnWithDataForUser(earliest);

		if (cid != noCID)
		{
			wns::SmartPtr<const wns::service::phy::phymode::PhyModeInterface> phyModePtr = userPhyModes[earliest];
			double rate = phyModePtr->getDataRate();
			simTimeType pduDuration = colleagues.queue->getHeadOfLinePDUbits(cid) / rate;

			MESSAGE_SINGLE(NORMAL, logger, "PDU size: " << colleagues.queue->getHeadOfLinePDUbits(cid)
				       << "\n PhyMode: " << phyModePtr->getString()
				       << "\n data rate of: " << rate
				       << "\n PDU Duration: " << std::setprecision (15) << pduDuration
				       << "\n timeMarker: " << std::setprecision (15)<< timeMarkers[earliest]
				       << "\n burstlength: " << burstLength
				       << "\n burstLengthMax: " << burstLengthMax);

			// \todo round to next OFDM symbol
			if (pduDuration + timeMarkers[earliest] <= burstLengthMax + slotLengthRoundingTolerance)
			{
				bool firstPDU = false;

				// first pdu in burst for interference measurement
				if (timeMarkers[earliest] == 0.0)
					firstPDU = true;

				wns::ldk::CompoundPtr pdu = colleagues.queue->getHeadOfLinePDU(cid);
                userBursts[earliest]->compounds.push_back(pdu);
                userBursts[earliest]->start = burstStart;
                userBursts[earliest]->user = earliest;
                userBursts[earliest]->subBand = subBand;
                userBursts[earliest]->spatialLayer = spatialLayerId[earliest];
                userBursts[earliest]->txPower = txPowerPerStream;
                userBursts[earliest]->phyModePtr = userPhyModes[earliest];
                userBursts[earliest]->pattern = patterns[earliest];
                userBursts[earliest]->estimatedCandI = group[earliest];
                userBursts[earliest]->phyModePtr = userPhyModes[earliest];

				timeMarkers[earliest] += pduDuration;
				bitsThisFrame[earliest] += pdu->getLengthInBits();

				this->oneBurstSucceeded = true; // mark this run as succeeded
				MESSAGE_SINGLE(NORMAL, logger, "after Scheduling, timeMarker = "<< timeMarkers[earliest]);

			} else { // we cannot schedule any further pdu for this user

				MESSAGE_SINGLE(NORMAL, logger, "No pdu for user "<< colleagues.registry->getNameForUser(earliest)<< " fits into phase");
				finished[earliest] = true;
			}
		}
		else
		{   // first user without data, end burst here
			// unless we are allowed to go on forever
			// (e.g. in case there is only one group with data left in the
			// scheduler)

			MESSAGE_SINGLE(NORMAL, logger, "No more pdus for user "<< colleagues.registry->getNameForUser(earliest));

			finished[earliest] = true; // mark this one as finished anyway

			if (!allowForever)
			{
				// allow users to fill up empty space, but not
				// to increase the burstLength any more
				burstLengthMax = timeMarkers[earliest];
				// break; // leave big do..while loop
			} else {

				MESSAGE_SINGLE(NORMAL, logger,
					       "But I was told to allow other users to go on");
			}
		}
	}
	while(!everybodyFinished(finished));

	// finalize burst infos and add them to the other burst descriptors for
	// this frame
	for (std::map<UserID, MapInfoEntryPtr>::iterator iter = userBursts.begin();
		 iter != userBursts.end(); ++iter)
	{
		// only write burst info for users that were actually scheduled
		if (timeMarkers[iter->first] > 0.0)
		{
			assure(timeMarkers[iter->first] <= burstLength + slotLengthRoundingTolerance, "Scheduled too much into this burst");
			iter->second->end = burstStart + timeMarkers[iter->first]; // set burst end for user

			///\todo Fixme: Don't save burst if already too many of them - this
			///might happen if scheduleOneBurst() was called when bursts.size()
			///was just below the limit

			//if (bursts.size() < maxBursts)
			// TODO [rs]: get rid of getSchedulerState()->currentState->bursts->size()
			if (getSchedulerState()->currentState->bursts->size() < maxBursts)
				bursts_push_back(iter->second); // and save them to real burst container

			// keep track of the end of this burst
			if (timeMarkers[iter->first] > currentBurstEnd)
				currentBurstEnd = timeMarkers[iter->first];
		}
	}
	assure(currentBurstEnd <= burstStart + burstLength + slotLengthRoundingTolerance, "Scheduled too much into the burst");
	return currentBurstEnd;
}

void
ProportionalFairDL::doStartScheduling(int subBands, int maxSpatialLayers, simTimeType slotLength)
{
	const simTimeType symbolDuration = getSchedulerState()->symbolDuration;
	MESSAGE_SINGLE(NORMAL, logger, "doStartScheduling(subBands="<<subBands<<", maxSpatialLayers="<<maxSpatialLayers<<", slotLength="<<slotLength);

	UserSet allUsersInQueue = colleagues.queue->getQueuedUsers();
	UserSet activeUsers     = colleagues.registry->filterReachable(allUsersInQueue);

	if (activeUsers.size() == 0){
		MESSAGE_SINGLE(NORMAL, logger, "no active user needs to be scheduled");
		return;
	}

	// add new users to allUsers, if not already in set:
	for (UserSet::const_iterator iter = allUsersInQueue.begin();
	     iter !=  allUsersInQueue.end(); ++iter)
		allUsers.insert(*iter);

	// init data structure to keep track of used time in subBands
	std::vector<simTimeType> nextFreeSubBand;
	nextFreeSubBand.clear();

	for (int i = 0; i < subBands; ++i)
		nextFreeSubBand.push_back(0.0);

	// (done in Strategy.cpp): clearMap(); //bursts.clear();

	// init bit counters for all users
	for (UserSet::const_iterator iter = allUsers.begin();
	     iter !=  allUsers.end(); ++iter)
		bitsThisFrame[*iter] = 0;

	wns::Power txPowerPerStream;

	unsigned int burstsBeforeRound;
	//bool somethingWasScheduled=false;
	int roundNumber=0; // just for debugging
	do // while some PDU scheduled
	{
		//somethingWasScheduled=false;
		burstsBeforeRound = getSchedulerState()->currentState->bursts->size(); // 0 at the beginning
		MESSAGE_SINGLE(NORMAL, logger, "startScheduling(round="<<roundNumber<<"): burstsBeforeRound=" << burstsBeforeRound);
		//MESSAGE_SINGLE(NORMAL, logger, "startScheduling(round="<<roundNumber<<")");

		// get only those users that are still active
		allUsersInQueue = colleagues.queue->getQueuedUsers();
		activeUsers     = colleagues.registry->filterReachable(allUsersInQueue);
		if (activeUsers.size() == 0)
			return;

		Grouping grouping = colleagues.grouper->getTxGrouping(activeUsers, maxSpatialLayers);
		// grouping contains only all possible groups here.

		MESSAGE_SINGLE(NORMAL, logger, "startScheduling(round="<<roundNumber<<"): retrieved grouping:\n" << grouping.getDebugOutput());

		// calculate group preference for every group and store them in a priority queue

		std::priority_queue<GroupPrefPair> preferences; // priority_queue automatically sorts
		assure(preferences.size()==0, "preferences.size() must be 0"); // new 03.12.2007

		// Calculate Preferences for groups and order them
		for (std::vector<Group>::const_iterator iter = grouping.groups.begin();
		     iter != grouping.groups.end(); ++iter)
		{
			float groupPref = getPreference(*iter); // preference=f(minRate,groupRate)
			MESSAGE_SINGLE(NORMAL, logger, "Saving "<<printGroup(*iter)<<" with preference "<< groupPref);
			preferences.push(GroupPrefPair(groupPref, *iter)); // automatic sorting
		}

		// while still groups in priority queue, get the group with the next
		// highest preference
		// TODO [rs]: get rid of getSchedulerState()->currentState->bursts.size()
		while (preferences.size() && (getSchedulerState()->currentState->bursts->size() < maxBursts))
		//while (preferences.size() && (bursts.size() < maxBursts))
		{
			// get first group from priority queue and remove it from queue
			const float pref  = preferences.top().first;
			const Group group = preferences.top().second;
			MESSAGE_SINGLE(NORMAL, logger, "Scheduling "<<printGroup(group)<<" with preference "<< pref<<":");

			// choose the subBand that has the next transmit opportunity
			// (next free subBand)
			int subBand = -1;
			if (slotLength - *min_element(nextFreeSubBand.begin(), nextFreeSubBand.end()) < symbolDuration)
				break; // no subband has free OFDM symbol
			else
				subBand	= min_element(nextFreeSubBand.begin(), nextFreeSubBand.end()) - nextFreeSubBand.begin();

			// if there is only one group left in the grouping, allow this group
			// to go on forever (well, until frame is full) regardless of whether
			// one of the users is already finished.
			// also allow this if this is the last group we can fit into the
			// frame because of the maxBursts limit
			bool allowForever = false;
			if ((grouping.groups.size() == 1) ||
			    (getSchedulerState()->currentState->bursts->size() + group.size() >= maxBursts)) {
				allowForever = true;
				MESSAGE_SINGLE(NORMAL, logger, "Allowing to go on forever");
			}

			// adapt TxPower of each stream of the group
			txPowerPerStream = getTxPower();
			assure(txPowerPerStream!=wns::Power(),"undefined power="<<txPowerPerStream);
			if(!isEirpLimited()){
				txPowerPerStream += grouping.shareOfPowerPerStreams(group);
				MESSAGE_SINGLE(NORMAL, logger, "share of power per concurrent stream: "
							   << grouping.shareOfPowerPerStreams(group).get_factor());
			} else { // if(eirpLimited)
				txPowerPerStream += grouping.eirpReductionOfPower(group);
				MESSAGE_SINGLE(NORMAL, logger, "Tx power reduction due to EIRP: "
							   << grouping.eirpReductionOfPower(group).get_factor());
			}

			this->oneBurstSucceeded = false; // reset flag to
							 // determine whether a
							 // burst was really scheduled
			simTimeType thisBurstDuration = scheduleOneBurst(
				nextFreeSubBand[subBand],  // start time for this burst
				slotLength - nextFreeSubBand[subBand], // max duration
				group, // the group for the slot
				subBand,
				grouping.patterns,
				allowForever,
				txPowerPerStream);

			assure(thisBurstDuration + nextFreeSubBand[subBand] <= slotLength + slotLengthRoundingTolerance, "burst too long");
			//somethingWasScheduled=true;
			// if something scheduled, save the end of this burst and round to the next new OFDM symbol
			if (thisBurstDuration > 0.0)
				nextFreeSubBand[subBand] += ceil(thisBurstDuration / symbolDuration) * symbolDuration;

			// unless we are allowed to regroup after every group. In this case,
			// forget about the other groups and get a new grouping
			if (allowReGrouping)
			{
				MESSAGE_SINGLE(NORMAL, logger, "Finished serving the group with the highest preference. Triggering immediate re-grouping...");
				break;
			}
			// keep this groups in the preferences list until it is exhausted
			// 'exhausted' means that for at least one member of the group
			// we no longer have data in the queue
			// we also do not consider this group any longer if it
			// can not be fit into what remains of any of freq channels
			if (groupExhausted(group) || this->oneBurstSucceeded==false)
				preferences.pop();
		} // else, go, schedule the next group
		roundNumber++; // just for debugging
	//} while (somethingWasScheduled);
	} while (getSchedulerState()->currentState->bursts->size() != burstsBeforeRound); // something was scheduled
	// update the past data rates for all users that we know of:
	for (UserSet::const_iterator iter = allUsers.begin();
	     iter != allUsers.end(); ++iter)
	{
		UserID user = *iter;
		Bits bitsInThisFrame;
		if (bitsThisFrame.find(user) != bitsThisFrame.end())
			bitsInThisFrame = bitsThisFrame[user];
		else // was not even active this frame
			bitsInThisFrame = 0;
		updatePastDataRates(user, bitsInThisFrame, slotLength);
	}

	double usedPercentage = 0.0;
	for (int subBand = 0; subBand < subBands; ++subBand)
		usedPercentage += nextFreeSubBand[subBand] / slotLength;
	this->resourceUsage = usedPercentage / subBands;
	MESSAGE_SINGLE(NORMAL, logger,"Resources allocated in this Frame ("<<subBands<<" subchannels): " << 100.0*this->resourceUsage << "%");
} // doStartScheduling



