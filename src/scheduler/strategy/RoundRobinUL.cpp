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

#include <WNS/scheduler/strategy/RoundRobinUL.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>
#include <WNS/scheduler/CallBackInterface.hpp>

#include <vector>
#include <map>
#include <algorithm>
#include <iostream>

using namespace wns::scheduler;
using namespace wns::scheduler::strategy;


STATIC_FACTORY_REGISTER_WITH_CREATOR(RoundRobinUL, StrategyInterface, "RoundRobinUL", wns::PyConfigViewCreator);

StrategyResult
doStartScheduling(SchedulerStatePtr schedulerState,
		  SchedulingMapPtr schedulingMap)
{
}

void
RoundRobinUL::doStartScheduling(int fChannels, int maxSpatialLayers, simTimeType slotLength)
{

	MESSAGE_SINGLE(NORMAL, logger,"RoundRobinUL::startScheduling called - Rx"
				   << "\n\t Channels:   " << fChannels
				   << "\n\t maxSpatialLayers:   " << maxSpatialLayers
				   << "\n\t slotLength: " << slotLength);

	assure(dynamic_cast<queue::QueueInterface*>(colleagues.queue), "Need access to the queue");
	assure(dynamic_cast<grouper::GroupingProviderInterface*>(colleagues.grouper), "Need access to the grouper");
	assure(dynamic_cast<RegistryProxyInterface*>(colleagues.registry), "Need access to the registry");

	//UserSet allUsers  = colleagues.registry->getActiveULUsers();
	UserSet	allUsers    = colleagues.queue->getQueuedUsers();
	//UserSet activeUsers = colleagues.registry->filterReachable(allUsers);

	if (allUsers.empty())
	{
		MESSAGE_SINGLE(NORMAL, logger, "RoundRobinUL: no Users in Queue. Stopping Scheduling Process:");
		return;
	}

	UserSet activeUsers = colleagues.registry->filterReachable(allUsers);

	if (activeUsers.empty())
	{
		MESSAGE_SINGLE(NORMAL, logger, "RoundRobinUL: no reachable active Users. Stopping Scheduling Process:");
		return;
	}

	// We are going to schedule a burst for every user
	Grouping grouping = colleagues.grouper->getRxGrouping(activeUsers, maxSpatialLayers);

	MESSAGE_SINGLE(NORMAL, logger,"RoundRobinUL::startScheduling Rx - retrieved grouping from grouper:\n" << grouping.getDebugOutput());

	// We give every group the same amount of overall blocks and
	// sequentially distribute them over all the subbands, first in the
	// f-direction, then in the t-direction. We try to allocate as many
	// sequential blocks into one burst as possible.
	uint32_t numGroups = grouping.groups.size();
	uint32_t blocksPerChannel = uint32_t( (slotLength + slotLengthRoundingTolerance) / this->blockDuration);
	uint32_t totalBlocks = fChannels * blocksPerChannel;
	uint32_t blocksPerGroup = uint32_t(totalBlocks / numGroups);

	assure(blocksPerGroup >= 1, "Not enough Resource Allocation Blocks or too many Spatial Groups to allow RoundRobinUL Scheduling");

	MESSAGE_SINGLE(NORMAL, logger,"RoundRobinUL::partitioned the UL Frame"
				   << "\n\t scheduling period per channel:   " << slotLength
				   << "\n\t block duration:   " << this->blockDuration
				   << "\n\t blocks per channel:   " << blocksPerChannel
				   << "\n\t total blocks in frame:   " << totalBlocks
				   << "\n\t blocks per group: " << blocksPerGroup);


	int currentSubBand = 0;
	simTimeType posInSubBand = 0.0;
	// (done in Strategy.cpp): clearMap(); // empty bursts

	// Iterate over all groups
	for (unsigned int group = 0; group < grouping.groups.size(); ++group) {
		Group currentGroup = grouping.groups[group];
		for (Group::iterator iter = currentGroup.begin();
			 iter != currentGroup.end(); ++iter) {
			// we can delete all other BWReqs for the user for this
			// round
			colleagues.queue->resetQueues(iter->first);
		}

		uint32_t blocksRemaining = blocksPerGroup;
		while (blocksRemaining != 0)
		{
			// create one burst per iteration
			assure(currentSubBand < fChannels, "Big confusion concerning total subBands and bursts");

			simTimeType burstStart = posInSubBand;
			simTimeType remaining = slotLength - burstStart;
			uint32_t numBlocks = uint32_t((remaining + slotLengthRoundingTolerance)/ this->blockDuration);
			MESSAGE_SINGLE(NORMAL, logger,"RoundRobinUL: Time   remaining in subBand "<<currentSubBand<<": "<<remaining + slotLengthRoundingTolerance);
			MESSAGE_SINGLE(NORMAL, logger,"RoundRobinUL: Blocks remaining in subBand "<<currentSubBand<<": "<<numBlocks);
			if (blocksRemaining < numBlocks)
			{
				// Only try to allocate the number of blocks
				// remaining for this group, even though more
				// might fit on this subBand
				numBlocks = blocksRemaining;
			}
			if (numBlocks==0)
			{
				// No block fits into the current subband -> Start over with next subband
				++currentSubBand;
				posInSubBand = 0.0;
			}
			else
			{
				// Some or all blocks fit into the remaining subband
				// Allocate as many subsequent blocks as fit
				// into one burst
				blocksRemaining -= numBlocks;
				simTimeType burstDuration = numBlocks * this->blockDuration;
				MESSAGE_SINGLE(NORMAL, logger,"Strategy RRUL: burstDuration=" << burstDuration);
				simTimeType burstEnd = burstStart + burstDuration;
				posInSubBand = burstEnd;
				assure( burstEnd <= slotLength + slotLengthRoundingTolerance, "RoundRobinUL: Burst end exceeds slotLength");
				MESSAGE_SINGLE(NORMAL, logger,"RoundRobinUL: Allocating Burst with "<<numBlocks<<" blocks on subBand "<<currentSubBand);
				int spatialLayer = 0;
				// for every user we provide one MapInfoEntry and tell the parent to set
				// the timingcommand for the dummy pdu so that the patterns get set
				for (Group::iterator iter = currentGroup.begin();
				     iter != currentGroup.end(); ++iter) {
					UserID user = iter->first;
					PowerCapabilities power = colleagues.registry->getPowerCapabilities(user);

					MapInfoEntryPtr currentBurst(new MapInfoEntry());

					wns::Ratio sinr(currentGroup[user].C / currentGroup[user].I);
					wns::service::phy::phymode::PhyModeInterfacePtr phyModePtr
						= this->getBestPhyMode(sinr);
					// currentBurst is a MapInfoEntry
					currentBurst->start = burstStart;
					currentBurst->end = burstEnd;
					currentBurst->user = user;
					currentBurst->subBand = currentSubBand;
					currentBurst->phyModePtr = phyModePtr;
					currentBurst->txPower = power.nominalPerSubband;
					currentBurst->estimatedCandI = currentGroup[user];

					MESSAGE_SINGLE(NORMAL, logger, "PHY mode is " << *phyModePtr<<" with SINR="<<sinr);

					///\todo FIXME: provider a cleaner handling for the situation that
					///one of the users in the grouping does not have a sufficient SINR
					///estimate for transmission
					if (phyModePtr->isValid())
					{
						bursts_push_back(currentBurst);

						wns::ldk::CompoundPtr pdu;

						MESSAGE_BEGIN(VERBOSE, logger, m, "RoundRobinUL: Scheduling Burst");
						m << "\n\tcurrentSubBand: " << currentSubBand
						  << "\n\tburstStart: " << burstStart
						  << "\n\tburstEnd: " << burstEnd
						  << "\n\tuser: " << user->getName()
						  << "\n\tspatialLayer: " << spatialLayer
						  << "\n\tphyMode: " << *phyModePtr
						  << "\n\ttxPower: " << getTxPower()
						  << "\n\tcarrierEst: " << currentGroup[user].C
						  << "\n\tinterfEst: " << currentGroup[user].I;
						MESSAGE_END();

						compoundReady(currentSubBand,
							      burstStart,
							      burstEnd,
							      user,
							      pdu,
							      spatialLayer,
							      grouping.patterns[user],
							      currentBurst,
							      *phyModePtr,
							      power.nominalPerSubband,
							      currentGroup[user]);
						spatialLayer++;
					} else {
						MESSAGE_SINGLE(NORMAL, logger,"RR UL scheduler ignoring user with zero PhyMode");
					}
				}
			}
		}
	}
}





