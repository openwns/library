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

#include <WNS/scheduler/strategy/CQIEnabledRoundRobinUL.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>
#include <WNS/scheduler/CallBackInterface.hpp>

#include <vector>
#include <map>
#include <algorithm>
#include <iostream>

using namespace wns::scheduler;
using namespace wns::scheduler::strategy;


STATIC_FACTORY_REGISTER_WITH_CREATOR(CQIEnabledRoundRobinUL, StrategyInterface, "CQIEnabledRoundRobinUL", wns::PyConfigViewCreator);

void
CQIEnabledRoundRobinUL::onColleaguesKnown()
{
	Strategy::onColleaguesKnown();
	phyModeMapper = colleagues.registry->getPhyModeMapper();
	assure(phyModeMapper != NULL, "invalid phyModeMapper");
}

wns::Power
CQIEnabledRoundRobinUL::getRemainedTxPower(int /*channel*/, double beginTime, wns::Power maxSummedPowerOnAllChannels, usedTxPowerOnAllChannels& usedTxPower)
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

void
CQIEnabledRoundRobinUL::doStartScheduling(int fChannels, int maxSpatialLayers, simTimeType slotLength)
{
	MESSAGE_SINGLE(NORMAL, logger,"CQIEnabledRoundRobinUL::startScheduling("<< "fChannels="<<fChannels<< ",maxSpatialLayers="<<maxSpatialLayers<<",slotLength="<<slotLength<<")");

	assure(dynamic_cast<queue::QueueInterface*>(colleagues.queue), "Need access to the queue");
	assure(dynamic_cast<grouper::GroupingProviderInterface*>(colleagues.grouper), "Need access to the grouper");
	assure(dynamic_cast<RegistryProxyInterface*>(colleagues.registry), "Need access to the registry");

	UserSet allUsersInQueue = colleagues.queue->getQueuedUsers();
	if (allUsersInQueue.empty()) { return; }
	UserSet activeUsers = colleagues.registry->filterReachable(allUsersInQueue);
	if (activeUsers.empty()) { return; }

	// We are going to schedule a burst for every user
	Grouping grouping = colleagues.grouper->getRxGrouping(activeUsers, maxSpatialLayers);

	// We give every group the same amount of overall blocks and
	//distribute them over all the subbands,
	uint32_t numGroups = grouping.groups.size();
	uint32_t blocksPerChannel = uint32_t(slotLength / this->blockDuration);
	//uint32_t blocksPerChannel = uint32_t( (slotLength + slotLengthRoundingTolerance) / this->blockDuration); // TODO
	uint32_t totalBlocks = fChannels * blocksPerChannel;
	uint32_t blocksPerGroup = uint32_t(totalBlocks / numGroups);

	assure(blocksPerGroup >= 1, "Not enough Resource Allocation Blocks or too many Spatial Groups to allow CQIEnabledRoundRobinUL Scheduling");

	MESSAGE_SINGLE(NORMAL, logger,"CQIEnabledRoundRobinUL::partitioned the UL Frame"
		       << "\n\t scheduling period per channel:   " << slotLength
		       << "\n\t block duration:   " << this->blockDuration
		       << "\n\t blocks per channel:   " << blocksPerChannel
		       << "\n\t total blocks in frame:   " << totalBlocks
		       << "\n\t blocks per group: " << blocksPerGroup);

	if(useNominalTxPower)
	{
		MESSAGE_SINGLE(NORMAL, logger, "we are told to always use nominal txpower, so no adaptive power control here!");
	}

	// (done in Strategy.cpp): clearMap(); // empty bursts

	//here we mark each resource block if it is used
	bool **used = new bool*[fChannels];
	for(int i = 0; i < fChannels; i++)
	{
		used[i] = new bool[blocksPerChannel];
		for(uint32_t j = 0; j < blocksPerChannel; j++ )
		{
			used[i][j] = false;
		}
	}

	// Iterate over all groups
	for (unsigned int group = 0; group < grouping.groups.size(); ++group) {
		Group currentGroup = grouping.groups[group];

		assure(currentGroup.size() <= 1, "Now CQIEnabledRoundRobinUL only works for one user grouping ");

		if(currentGroup.empty())
		{
		    continue;
		}

		UserID userInGroup = currentGroup.begin()->first;
		// after obtaining one BWReq compound from this user, we
		// can delete all other BWReqs for the user for this
		// round
		colleagues.queue->resetQueues(userInGroup);

		//we need to record the txPower of the user to ensure it is not beyond the limit
		usedTxPowerOnAllChannels usedTxPower(fChannels);

		//now we are doing the schedulering work in BS for UT and FRS.
		PowerCapabilities power = colleagues.registry->getPowerCapabilities(userInGroup);
		wns::Power maxSummedPower = power.maxOverall;
		wns::Power maxPowerPerSubChannel = power.maxPerSubband;
		wns::Power nominalPowerPerSubChannel = power.nominalPerSubband;

		MESSAGE_SINGLE(NORMAL, logger,"PowerLimit for "<<colleagues.registry->getNameForUser(userInGroup)<<": maxSummedPower: "<<maxSummedPower<<" maxPowerPerSubChannel="<< maxPowerPerSubChannel<<" nominalPowerPerSubChannel="<<nominalPowerPerSubChannel);

		uint32_t blocksRemaining = blocksPerGroup;
		// SmartPtr:
		ChannelQualitiesOnAllSubBandsPtr channelQualities = colleagues.registry->getChannelQualities4UserOnUplink(userInGroup, 0/*frameNr*/);

		// sometimes we can not get the channel qualities, like: due to some specifical design of WNS, BS may need to do scheduling for RN without sending/receiveing any PDU to/from it
		if ((channelQualities!=ChannelQualitiesOnAllSubBandsPtr())
		    && (channelQualities->size() > 0))
		{
			// Warning: This works on the original datastructure and has O(N*log(N)^2) !:
			std::stable_sort(channelQualities->begin(), channelQualities->end(), betterChannelQuality());
			MESSAGE_SINGLE(NORMAL, logger,"We got channel qualities for "<<colleagues.registry->getNameForUser(userInGroup)<<", The Scheduling result is:" );

			//we choose from the best channel to the worst
			for(unsigned int i = 0; i < channelQualities->size(); i++)
			{
				if(blocksRemaining <= 0)
				{
					break;
				}

				int currentChannel = i;
				if((currentChannel < 0)||(currentChannel >= fChannels))
				{
				    continue;
				}

				for(uint32_t j = 0; j < blocksPerChannel; j++ )
				{
					if(used[currentChannel][j] == false)
					{
						simTimeType burstStart = j * blockDuration;
						uint32_t numBlocks = blocksPerChannel - j;

						//the number of  remaining blocks of this user is less  than the rest bloks on this channel
						if (blocksRemaining < numBlocks)
						{
							// Only try to allocate the number of blocks remaining for this group, even though more might fit on this subBand
							numBlocks = blocksRemaining;
						}

						blocksRemaining -= numBlocks;
						simTimeType burstDuration = numBlocks * this->blockDuration;
						simTimeType burstEnd = burstStart + burstDuration;
						assure( burstDuration>0, "burstDuration="<<burstDuration<<", numBlocks="<<numBlocks<<", blockDuration="<<this->blockDuration);

						assure( burstEnd <= slotLength + slotLengthRoundingTolerance, "CQIEnabledRoundRobinUL: Burst end exceeds slotLength");

						wns::Power requiredTxPower;
						wns::SmartPtr<const wns::service::phy::phymode::PhyModeInterface> phyModePtr;

						if(useNominalTxPower)
						{
						  ChannelQualityOnOneSubChannel channelQualityOnBestChannel = (*channelQualities)[i];
							requiredTxPower = nominalPowerPerSubChannel;
							wns::Ratio rxSINR = requiredTxPower/(channelQualityOnBestChannel.interference * channelQualityOnBestChannel.pathloss.get_factor());
							phyModePtr = (phyModeMapper->getBestPhyMode(rxSINR));
						}
						else
						{
							wns::Power remainedTxPowerOnAllChannels = getRemainedTxPower(currentChannel, burstStart,maxSummedPower,usedTxPower);

							wns::Power maxTxPower = (maxPowerPerSubChannel > remainedTxPowerOnAllChannels ? remainedTxPowerOnAllChannels : maxPowerPerSubChannel );

							ChannelQualityOnOneSubChannel channelQualityOnBestChannel = (*channelQualities)[i];
							wns::Ratio maxSINR = maxTxPower/(channelQualityOnBestChannel.interference * channelQualityOnBestChannel.pathloss.get_factor());
							phyModePtr = (phyModeMapper->getBestPhyMode(maxSINR));
							//we always try to use the minimal tx power for ceratin phymode to save power
							wns::Ratio minSINR = phyModeMapper->getMinSINRRatio(phyModePtr);
							requiredTxPower = wns::Power::from_mW(minSINR.get_factor() * channelQualityOnBestChannel.pathloss.get_factor() * channelQualityOnBestChannel.interference.get_mW() ) ;
						}

						assure(phyModePtr!=wns::service::phy::phymode::PhyModeInterfacePtr(),"phyModePtr=NULL");
						int spatialLayer = 0;

						// for every user we provide one MapInfoEntry and tell the parent to set
						// the timingcommand for the dummy pdu so that the patterns get set

						UserID user = currentGroup.begin()->first;
						MapInfoEntryPtr currentBurst(new MapInfoEntry());

						currentBurst->start = burstStart;
						currentBurst->end = burstEnd;
						assure(burstEnd > burstStart, "burstStart="<<burstStart<<" and burstEnd="<<burstEnd);
						currentBurst->user = user;
						currentBurst->subBand = currentChannel;
						currentBurst->phyModePtr = phyModePtr;
						currentBurst->txPower = requiredTxPower;
						currentBurst->estimatedCandI = currentGroup[user];

						///\todo FIXME: provider a cleaner handling for the situation that
						///one of the users in the grouping does not have a sufficient SINR
						///estimate for transmission
						if (phyModePtr->isValid() && requiredTxPower != wns::Power())
						{

							bursts_push_back(currentBurst);

							wns::ldk::CompoundPtr pdu;

							compoundReady(currentChannel,
								      burstStart,
								      burstEnd,
								      user,
								      pdu,
								      spatialLayer,
								      grouping.patterns[user],
								      currentBurst,
								      *phyModePtr,
								      requiredTxPower,
								      currentGroup[user]);

							MESSAGE_SINGLE(NORMAL, logger,"Channel: "<<currentChannel<<" PhyMode: "<<  *phyModePtr<<" txPower: "<<requiredTxPower<<" startT: "<<burstStart<<" endT: "<<burstEnd);

							for(uint32_t k = j; k < numBlocks; k++)
							{
								used[currentChannel][k] = true;
							}

							TxPower4PDU txPowerUsage(burstStart, burstEnd,requiredTxPower);
							usedTxPower[currentChannel].push_back(txPowerUsage);

						}

						//we don't need to check the rest blocks in this channel: either they are all used, or we don't need them
						break;
					}
				}
			}

		}
		else //no channel qualities infomation for this user, we just choose the first block to it.
		{
			MESSAGE_SINGLE(NORMAL, logger,"No channel qualities for "<<colleagues.registry->getNameForUser(userInGroup)<<", The Schedulering result is:" );

                        //we try to randomly distribute the channels to user, to get better channel quality

			for(int i = 0; i < fChannels; i++)
			{
				if(blocksRemaining <= 0)
				{
					break;
				}

				int currentChannel = i;
				for(uint32_t j = 0; j < blocksPerChannel; j++)
				{
					if(used[currentChannel][j] == false)
					{
						simTimeType burstStart = j * blockDuration;
						uint32_t numBlocks = blocksPerChannel - j;

						if (blocksRemaining < numBlocks)
						{
							// Only try to allocate the number of blocks remaining for this group, even though more might fit on this subBand
							numBlocks = blocksRemaining;
						}

						// Some or all blocks fit into the remaining subband Allocate as many subsequent blocks as fit into one burst
						blocksRemaining -= numBlocks;
						simTimeType burstDuration = numBlocks * this->blockDuration;
						simTimeType burstEnd = burstStart + burstDuration;

						assure( burstEnd <= slotLength + slotLengthRoundingTolerance, "RoundRobinUL: Burst end exceeds slotLength");

						wns::Power requiredTxPower;

						if(useNominalTxPower)
						{
							requiredTxPower = nominalPowerPerSubChannel;
						}
						else
						{
							wns::Power remainedTxPowerOnAllChannels = getRemainedTxPower(currentChannel, burstStart, maxSummedPower,usedTxPower);
							requiredTxPower = (nominalPowerPerSubChannel > remainedTxPowerOnAllChannels ? remainedTxPowerOnAllChannels : nominalPowerPerSubChannel );
						}

						int spatialLayer = 0;
						// for every user we provide one MapInfoEntry and tell the parent to set
						// the timingcommand for the dummy pdu so that the patterns get set

						UserID user = currentGroup.begin()->first;

						MapInfoEntryPtr currentBurst(new MapInfoEntry());

						wns::Ratio sinr(currentGroup[user].C / currentGroup[user].I);
						wns::SmartPtr<const wns::service::phy::phymode::PhyModeInterface> phyMode = phyModeMapper->getBestPhyMode(sinr);

						currentBurst->start = burstStart;
						currentBurst->end = burstEnd;
						currentBurst->user = user;
						currentBurst->subBand = currentChannel;
						currentBurst->phyModePtr = phyMode;
						currentBurst->txPower = requiredTxPower;
						currentBurst->estimatedCandI = currentGroup[user];

						///\todo FIXME: provider a cleaner handling for the situation that
						///one of the users in the grouping does not have a sufficient SINR
						///estimate for transmission
						if (phyMode->isValid() && requiredTxPower != wns::Power())
						{
							bursts_push_back(currentBurst);

							wns::ldk::CompoundPtr pdu;

							compoundReady(currentChannel,
								      burstStart,
								      burstEnd,
								      user,
								      pdu,
								      spatialLayer,
								      grouping.patterns[user],
								      currentBurst,
								      *phyMode,
								      requiredTxPower,
								      currentGroup[user]);

							MESSAGE_SINGLE(NORMAL, logger,"Channel: "<<currentChannel<<" PhyMode: "<< phyMode<<" txPower: "<<requiredTxPower<<" startT: "<<burstStart<<" endT: "<<burstEnd);

							for(uint32_t k = j; k < numBlocks; k++)
							{
								used[currentChannel][k] = true;
							}

							TxPower4PDU txPowerUsage(burstStart, burstEnd,requiredTxPower);
							usedTxPower[currentChannel].push_back(txPowerUsage);
						}

						//we don't need to check the rest blocks in this channel: either they are all used, or we don't need them
						break;
					}
				}
			}
		} // no CQI
	} // Iterate over all groups

	//release the memory
	for(int i = 0; i < fChannels; i++)
	{
		delete [] used[i];
	}
	delete [] used;
}





