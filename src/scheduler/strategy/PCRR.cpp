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

#include <WNS/scheduler/strategy/PCRR.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>
#include <WNS/scheduler/CallBackInterface.hpp>

#include <vector>
#include <map>
#include <algorithm>
#include <iostream>

using namespace wns::scheduler;
using namespace wns::scheduler::strategy;


STATIC_FACTORY_REGISTER_WITH_CREATOR(PCRR, StrategyInterface, "PCRR", wns::PyConfigViewCreator);


void
PCRR::doStartScheduling(int fChannels, int maxSpatialLayers, simTimeType slotLength)
{
	this->resourceUsage = 0.0;

	MESSAGE_SINGLE(NORMAL, logger,"PCRR::startScheduling called - Rx"
		       << "\n\t Channels:   " << fChannels
		       << "\n\t maxSpatialLayers:   " << maxSpatialLayers
		       << "\n\t slotLength: " << slotLength);

	assure(dynamic_cast<queue::QueueInterface*>(colleagues.queue), "Need access to the queue");
	assure(dynamic_cast<grouper::GroupingProviderInterface*>(colleagues.grouper), "Need access to the grouper");
	assure(dynamic_cast<RegistryProxyInterface*>(colleagues.registry), "Need access to the registry");

	//UserSet allUsers      = colleagues.registry->getActiveULUsers();
	UserSet allUsersInQueue = colleagues.queue->getQueuedUsers();
	//UserSet activeUsers     = colleagues.registry->filterReachable(allUsersInQueue);

	if (allUsersInQueue.empty())
	{
		MESSAGE_SINGLE(NORMAL, logger, "PCRR: no Users in Queue. Stopping Scheduling Process:");
		return;
	}

	//UserSet activeUsers = colleagues.registry->filterReachable(allUsers);
	UserSet activeUsers = colleagues.registry->filterReachable(allUsersInQueue);

	if (activeUsers.empty())
	{
		MESSAGE_SINGLE(NORMAL, logger, "PCRR: no active Users. Stopping Scheduling Process:");
		return;
	}

	Grouping grouping = colleagues.grouper->getRxGrouping(activeUsers, maxSpatialLayers);
	MESSAGE_SINGLE(NORMAL, logger,"PCRR::startScheduling Rx - retrieved grouping from grouper:\n" << grouping.getDebugOutput());

	PowerMap powerMap = colleagues.registry->calcULResources(activeUsers, fChannels);
	MESSAGE_SINGLE(NORMAL, logger,"PCRR::startScheduling Rx - retrieved UL Power and Resource Map\n");




	// We give every group the same amount of overall blocks and
	// sequentially distribute them over all the subbands, first in the
	// f-direction, then in the t-direction. We try to allocate as many
	// sequential blocks into one burst as possible.
	unsigned long int blocksPerChannel = (unsigned long int)(((1000 * slotLength / this->blockDuration) + 0.5)/1000);
	unsigned long int blocksPerSpatialLayer = fChannels * blocksPerChannel;
#if !defined(WNS_NO_LOGGING) || !defined(NDEBUG)
	unsigned long int totalBlocks = maxSpatialLayers * blocksPerSpatialLayer;
#endif
	MESSAGE_SINGLE(NORMAL, logger,"PCRR::partitioned the UL Frame"
		       << "\n\t scheduling period per channel:   " << slotLength
		       << "\n\t block duration:   " << this->blockDuration
		       << "\n\t blocks per channel:   " << blocksPerChannel
		       << "\n\t total blocks per spatial layer:   " << blocksPerSpatialLayer
		       << "\n\t total blocks in frame:   " << totalBlocks);

	// Try to fairly distribute "totalBlocks" among the active users,
	// considering their requests
	std::vector<double> sharePerGroup(grouping.groups.size(), 0.0);
	std::vector<unsigned long int> blocksPerGroup(grouping.groups.size(), 0);
	unsigned long int sumAllocatedBlocks = 0;


	double sumShares = 0.0;
	// Step 1. Determine max share requested by group
	for (unsigned int group = 0; group < grouping.groups.size(); ++group) {
		Group currentGroup = grouping.groups[group];
		double maxShare = 0.0;
		for (Group::iterator iter = currentGroup.begin();
		     iter != currentGroup.end(); ++iter) {
			UserID user = iter->first;
			maxShare = std::max<double>(maxShare, powerMap[user].resourceShare);
		}
		sumShares += maxShare;
		sharePerGroup.at(group) = maxShare;
	}

	double controlSum = 0.0;
	// Step 2. Normalize the requested shares
	for (unsigned int group = 0; group < grouping.groups.size(); ++group) {
		sharePerGroup.at(group) = sharePerGroup.at(group) / sumShares;
		controlSum += sharePerGroup.at(group);
	}
	assure(std::abs(controlSum-1.0) < 1e-5, "After Normalization: Sum of requested shares is not 100%, but " << controlSum * 100 << "%");

	// Step 3. Allocate Blocks to groups
	std::list<GroupAlloc> allocs;
	for (unsigned int group = 0; group < grouping.groups.size(); ++group) {
		Group currentGroup = grouping.groups[group];

		GroupAlloc a;
		a.group = group;
		// each group gets the requested share, but 1 block at minimum
		unsigned long int thisGroupBlocks = std::max<unsigned long int>(1, (unsigned long int)(sharePerGroup.at(group) * blocksPerSpatialLayer));
		MESSAGE_SINGLE(NORMAL, logger, "Group " << group << " requests "
			       << sharePerGroup.at(group)*100 << " percent of the resources =" << thisGroupBlocks <<" blocks");

		for (Group::iterator iter = currentGroup.begin();
		     iter != currentGroup.end(); ++iter) {
			UserID user = iter->first;
			// reduce nr. of blocks to not exceed the max nr. of subbands
			if (double(thisGroupBlocks)/double(blocksPerChannel) > powerMap[user].maxNumSubbands)
			{
				MESSAGE_BEGIN(NORMAL, logger, m, "");
				m << "Max Nr of Blocks for Group " << group << "("<< thisGroupBlocks <<")"
				  << " bounded by maximum Nr of subbands for user "<< user.getName() << "("
				  << powerMap[user].maxNumSubbands
				  << ") --> using " << powerMap[user].maxNumSubbands * blocksPerChannel << " blocks.";
				MESSAGE_END();
				thisGroupBlocks = powerMap[user].maxNumSubbands * blocksPerChannel;
			}
		}

		a.blocks = thisGroupBlocks;
		allocs.push_back(a);

		sumAllocatedBlocks += thisGroupBlocks;
		blocksPerGroup.at(group) = thisGroupBlocks;
		MESSAGE_SINGLE(NORMAL, logger, "Scheduling "<<thisGroupBlocks<<" blocks for group " << group);
	}

	// Step 4.
	// we have to "socially" take away from those that have many
	// blocks and keep at least one block for the poor.
	while (sumAllocatedBlocks>blocksPerSpatialLayer)
	{
		// this sorts the list in ascending order according to the # of blocks
		allocs.sort(wns::scheduler::strategy::LtAlloc);

		unsigned long int biggestChunk = allocs.back().blocks;
		assure(biggestChunk>1, "No way to further reduce allocation share.");

		for (size_t ii = 0; ii<allocs.size(); ++ii)
		{
			unsigned long int thisChunk = allocs.back().blocks;
			if (thisChunk>1)
			{
				allocs.back().blocks = biggestChunk-1;
				blocksPerGroup.at(allocs.back().group) = allocs.back().blocks;
			}
			GroupAlloc tmp = allocs.back();
			allocs.pop_back();
			allocs.push_front(tmp);
		}
		sumAllocatedBlocks = countSumBlocks(allocs);
	}

	assure(sumAllocatedBlocks<=totalBlocks, "Trying to allocate" << sumAllocatedBlocks << " blocks, while only " << blocksPerSpatialLayer << " are available");
	MESSAGE_SINGLE(NORMAL, logger, "Scheduling "<< sumAllocatedBlocks<<" in total, leaving " << blocksPerSpatialLayer-sumAllocatedBlocks<<" unused");
	this->resourceUsage = float(sumAllocatedBlocks) / float(blocksPerSpatialLayer);
	MESSAGE_SINGLE(NORMAL, logger,"Resources allocated in this Frame ("<<fChannels<<" subchannels): " << 100.0*this->resourceUsage << "%");


	int currentSubBand = 0;
	simTimeType posInSubBand = 0.0;
	// (done in Strategy.cpp): clearMap(); // empty bursts

	// Iterate over all groups
	for (unsigned int group = 0; group < grouping.groups.size(); ++group) {
		//bwReqs.clear();
		Group currentGroup = grouping.groups[group];
		for (Group::iterator iter = currentGroup.begin();
			 iter != currentGroup.end(); ++iter) {
			// we can delete all other BWReqs for the user for this
			// round
			colleagues.queue->resetQueues(iter->first);
		}

		unsigned long int blocksRemaining = blocksPerGroup.at(group);
		while (blocksRemaining != 0)
		{
			// create one burst per iteration
			assure(currentSubBand < fChannels, "Big confusion concerning total subBands and bursts");
			simTimeType burstStart = posInSubBand;
			simTimeType remaining = slotLength - burstStart;
			unsigned long int numBlocks = (unsigned long int)((remaining + slotLengthRoundingTolerance)/ this->blockDuration);
			MESSAGE_SINGLE(NORMAL, logger,"PCRR: Time   remaining in subBand "<<currentSubBand<<": "<<remaining + slotLengthRoundingTolerance);
			MESSAGE_SINGLE(NORMAL, logger,"PCRR: Blocks remaining in subBand "<<currentSubBand<<": "<<numBlocks);
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
				simTimeType burstEnd = burstStart + burstDuration;
				posInSubBand = burstEnd;
				assure( burstEnd <= slotLength + slotLengthRoundingTolerance, "PCRR: Burst end exceeds slotLength");
				MESSAGE_SINGLE(NORMAL, logger,"PCRR: Allocating Burst with "<<numBlocks<<" blocks on subBand "<<currentSubBand);
				int spatialLayer = 0;
				// for every user we provide one MapInfoEntry and tell the parent to set
				// the timingcommand for the dummy pdu so that the patterns get set
				for (Group::iterator iter = currentGroup.begin();
				     iter != currentGroup.end(); ++iter) {
					UserID user = iter->first;

					MapInfoEntryPtr currentBurst(new MapInfoEntry());

					wns::SmartPtr<const wns::service::phy::phymode::PhyModeInterface> phyMode = getBestPhyMode(currentGroup[user].C / currentGroup[user].I);

					currentBurst->start = burstStart;
					currentBurst->end = burstEnd;
					currentBurst->user = user;
					currentBurst->subBand = currentSubBand;
					currentBurst->txPower = powerMap[user].txPowerPerSubband;
					currentBurst->phyModePtr = phyMode;
                    currentBurst->estimatedCQI.interference = currentGroup[user].I;
                    currentBurst->estimatedCQI.carrier = currentGroup[user].C;

					///\todo FIXME: provider a cleaner handling for the situation that
					///one of the users in the grouping does not have a sufficient SINR
					///estimate for transmission
					if (currentBurst->phyModePtr->isValid())
					{
						bursts_push_back(currentBurst);

						wns::ldk::CompoundPtr pdu;

						MESSAGE_BEGIN(VERBOSE, logger, m, "PCRR: Scheduling Burst");
						m << "\n\tcurrentSubBand: " << currentSubBand
						  << "\n\tburstStart: " << burstStart
						  << "\n\tburstEnd: " << burstEnd
						  << "\n\tuser: " << user.getName()
						  << "\n\tspatialLayer: " << spatialLayer
						  << "\n\tPhyMode: " << *phyMode
						  << "\n\ttxPower: " << powerMap[user].txPowerPerSubband
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
							      *(phyMode),
							      powerMap[user].txPowerPerSubband,
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





