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

#include "EqualTimeRR.hpp"

#include <WNS/scheduler/CallBackInterface.hpp>

using namespace wns::scheduler;
using namespace wns::scheduler::strategy;

STATIC_FACTORY_REGISTER_WITH_CREATOR(EqualTimeRR,
				     StrategyInterface,
				     "EqualTimeRR",
				     wns::PyConfigViewCreator);


void
EqualTimeRR::doStartScheduling(int fChannels, int maxSpatialLayers, simTimeType slotLength)
{
	UserSet allUsersInQueue = colleagues.queue->getQueuedUsers();
	//UserSet activeUsers   = colleagues.registry->filterReachable(colleagues.queue->getQueuedUsers());
	UserSet activeUsers     = colleagues.registry->filterReachable(allUsersInQueue);
	if (activeUsers.size() == 0) return;

	// We are going to schedule a burst for every group

	Grouping grouping = colleagues.grouper->getTxGrouping(activeUsers, maxSpatialLayers);
	MESSAGE_SINGLE(NORMAL, logger, "EqualTimeRR::doStartScheduling(): Tx - retrieved grouping from grouper:\n" << grouping.getDebugOutput());

	// We give every group the same amount of time in one of the subBands.
	// Of course, the maximal burstlength is the slotlength, no multiple bursts
	// are scheduled for a group

	int burstsPerSubBand = int(ceil(float(grouping.groups.size()) / float(fChannels)));
	double burstLength = slotLength / double(burstsPerSubBand);
	int currentSubBand = 0;
	int posInSubBand = 0;
	// (done in Strategy.cpp): clearMap(); // empty bursts
	wns::Power txPowerPerStream;

	// every group is assigned exactly one burst-slot
	for (unsigned int group = 0; group < grouping.groups.size(); ++group) {
		assure(currentSubBand < fChannels, "Big confusion concerning total subBands and bursts");

		Group currentGroup = grouping.groups[group];

		///\todo align this to OFDM-symbol borders

		simTimeType burstStart = double(posInSubBand) * burstLength;
//		simTimeType burstEnd = burstStart + burstLength - slotLengthRoundingTolerance;
		int spatialLayer = 0;

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

		// every user of the group is scheduled into the group's time slot
		for (Group::iterator iter = currentGroup.begin();
		     iter != currentGroup.end(); ++iter) {
			UserID user = iter->first;

			//std::pair<PHYmode, DataRate> mode = getPhyMode(currentGroup[user].C / currentGroup[user].I);
			wns::Ratio sinr(currentGroup[user].C / currentGroup[user].I);
			wns::SmartPtr<const wns::service::phy::phymode::PhyModeInterface> phyMode = this->getBestPhyMode(sinr);
			MESSAGE_SINGLE(NORMAL, logger, "PHY mode estimation for: " << colleagues.registry->getNameForUser(user)
						   << " SINR: " << sinr
						   << " -> PHYMode "<< *phyMode);

			// now schedule
			wns::ldk::CompoundPtr pdu;
			simTimeType accumulatedTime = 0.0;
			bool firstPDU = true;

			// get all registered connctions for current user
			ConnectionVector allRegisteredConns = colleagues.registry->getConnectionsForUser(user);

			// Create the MapInfo Container
			MapInfoEntryPtr currentBurst(new MapInfoEntry());

			// then exhaustively empty all queues containing data until no more
			// data or burst full
			for (ConnectionVector::iterator iter = allRegisteredConns.begin();
				 iter != allRegisteredConns.end();
				 ++iter)
			{
				int cid = *iter;
				while (colleagues.queue->queueHasPDUs(cid))
				{
					simTimeType pduDuration =
						colleagues.queue->getHeadOfLinePDUbits(cid) / phyMode->getDataRate();

					// schedule pdu if it fits
					if (pduDuration + accumulatedTime < burstLength)
					{
						pdu = colleagues.queue->getHeadOfLinePDU(cid);
						currentBurst->compounds.push_back(pdu);
						compoundReady(currentSubBand,
									  burstStart + accumulatedTime, // startTime of PDU
									  burstStart + accumulatedTime + pduDuration, // endTime of PDU
									  user,
									  pdu, // the PDU
									  spatialLayer, // also for plotting
									  grouping.patterns[user], // the BF pattern
									  currentBurst, // the burst this PDU belongs to
									  *phyMode,
									  txPowerPerStream,
									  currentGroup[user]);
						accumulatedTime += pduDuration;
						firstPDU = false;
					}
					else  // HoL PDU doesn't fit, try next queue
						break;
				}
				// try next queue
			}
			// could not schedule any further pdus for this user in the current
			// burst, so for him the burst is over; write the mapInfo

			currentBurst->start = burstStart;
			currentBurst->end = burstStart + accumulatedTime;
			currentBurst->user = user;
			currentBurst->subBand = currentSubBand;
			currentBurst->phyModePtr = phyMode;
			bursts_push_back(currentBurst);


			spatialLayer++;
			// now schedule the user group members with differnt spatialLayers
		}

		// calculate the location of the next Burst
		posInSubBand++;
		if (posInSubBand == burstsPerSubBand) {
			posInSubBand = 0;
			currentSubBand++;
		}
	} // forall groups
}
