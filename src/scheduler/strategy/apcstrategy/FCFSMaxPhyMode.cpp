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

#include <WNS/scheduler/strategy/apcstrategy/FCFSMaxPhyMode.hpp>
#include <WNS/scheduler/strategy/apcstrategy/APCStrategy.hpp>
#include <WNS/scheduler/strategy/StrategyInterface.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>
#include <vector>
#include <iostream>
#include <algorithm>

using namespace wns::scheduler;
using namespace wns::scheduler::strategy;
using namespace wns::scheduler::strategy::apcstrategy;

STATIC_FACTORY_REGISTER_WITH_CREATOR(FCFSMaxPhyMode,
									 APCStrategyInterface,
									 "FCFSMaxPhyMode",
									 wns::PyConfigViewCreator);

FCFSMaxPhyMode::FCFSMaxPhyMode(const wns::pyconfig::View& config)
	: APCStrategy(config)
{
}

FCFSMaxPhyMode::~FCFSMaxPhyMode()
{
}

// called before each timeSlot/frame
void
FCFSMaxPhyMode::initialize(SchedulerStatePtr schedulerState,
			   SchedulingMapPtr schedulingMap)
{
	APCStrategy::initialize(schedulerState,schedulingMap); // must always initialize base class too
	MESSAGE_SINGLE(NORMAL, logger, "UseNominalTxPower::initialize("<<apcstrategyName<<")");
} // initialize

APCResult
FCFSMaxPhyMode::doStartAPC(RequestForResource& request,
			   SchedulerStatePtr schedulerState,
			   SchedulingMapPtr schedulingMap)
{
	APCResult apcResult;
	wns::scheduler::PowerCapabilities powerCapabilities =
		schedulerState->strategy->getPowerCapabilities(request.user);

	wns::Ratio pathloss     = request.cqiOnSubChannel.pathloss;
	wns::Power interference = request.cqiOnSubChannel.interference;

	assure(request.subChannel>=0,"need a valid subChannel");

	if (schedulerState->defaultTxPower!=wns::Power())
	{ // predefined, e.g. in slave mode
		apcResult.txPower = schedulerState->defaultTxPower;
		apcResult.sinr = apcResult.txPower/(interference*pathloss);
		apcResult.estimatedCandI = wns::CandI(apcResult.txPower/pathloss,interference);
		apcResult.phyModePtr = schedulerState->defaultPhyModePtr;
	} else {
		wns::Power totalPower = powerCapabilities.maxOverall;
		wns::Power remainingTxPowerOnAllSubChannels = schedulingMap->getRemainingPower(totalPower);

		if (remainingTxPowerOnAllSubChannels == wns::Power())
		{
			apcResult.txPower = wns::Power();
			return apcResult;
		}

		wns::Power maxPowerPerSubChannel = powerCapabilities.maxPerSubband;
		wns::Power maxTxPower =  (maxPowerPerSubChannel > remainingTxPowerOnAllSubChannels ? remainingTxPowerOnAllSubChannels :maxPowerPerSubChannel);
		wns::Ratio maxSINR = maxTxPower/(interference * pathloss);
		apcResult.phyModePtr = phyModeMapper->getBestPhyMode(maxSINR);

		//we always try to use the minimal tx power for ceratin phymode to save power
		wns::Ratio minSINR = phyModeMapper->getMinSINRRatio(apcResult.phyModePtr);
		apcResult.txPower = wns::Power::from_mW(minSINR.get_factor() * pathloss.get_factor() * interference.get_mW() ) ;
		apcResult.sinr = minSINR;
		apcResult.estimatedCandI = wns::CandI(apcResult.txPower/pathloss,interference);
	}
	MESSAGE_SINGLE(NORMAL, logger,"doStartAPC("<<request.toString()<<"): "
				   <<"SINR="<<apcResult.sinr<<", PhyMode="<<*(apcResult.phyModePtr));

	/*
	wns::Power maxPowerPerSubChannel = powerCapabilities.maxPerSubband;
	wns::Power maxSummedPowerOnAllChannels = powerCapabilities.maxOverall;
	wns::Power nominalPowerPerSubChannel = powerCapabilities.nominalPerSubband;

	if(schedulingPar.channelQualities.size() > 0)
	{
		assure(dynamic_cast<RegistryProxyInterface*>(colleagues.registry), "Need access to the registry");

		// perform power control here...
		wns::Power remainingTxPowerOnAllChannels = colleagues.registry->getRemainingTxPower(schedulingMap,schedulingPar);

		if (remainingTxPowerOnAllChannels == wns::Power())
		{
			connectionChr.txPower = wns::Power();
			return;
		}
		wns::Power maxTxPower = (maxPowerPerSubChannel > remainingTxPowerOnAllChannels ? remainingTxPowerOnAllChannels :maxPowerPerSubChannel);
		wns::Ratio maxSINR = maxTxPower/(schedulingPar.channelQualityOnBestChannel.Interference * schedulingPar.channelQualityOnBestChannel.Pathloss.get_factor());
		connectionChr.phyModePtr = phyModeMapper->getBestPhyMode(maxSINR);

		//we always try to use the minimal tx power for ceratin phymode to save power
		wns::Ratio minSINR = phyModeMapper->getMinSINRRatio(*(connectionChr.phyModePtr));

		connectionChr.txPower = wns::Power::from_mW(minSINR.get_factor() * schedulingPar.channelQualityOnBestChannel.Pathloss.get_factor() * schedulingPar.channelQualityOnBestChannel.Interference.get_mW() ) ;
		connectionChr.sinr = minSINR;
	}
	else
	{
		assure(dynamic_cast<RegistryProxyInterface*>(colleagues.registry), "Need access to the registry");

		// adapt TxPower of each stream of the group
		wns::Power txPowerPerStream = getTxPower4OneOfGroup(schedulingPar.grouping, schedulingPar.group);

		// perform power control here...
		wns::Power remainingTxPowerOnAllChannels = colleagues.registry->getRemainingTxPower(schedulingMap,schedulingPar);

		connectionChr.txPower = (txPowerPerStream > remainingTxPowerOnAllChannels ? remainingTxPowerOnAllChannels : txPowerPerStream );

		wns::Ratio sinr(connectionChr.estimatedCandI.C / connectionChr.estimatedCandI.I);

		connectionChr.sinr = sinr;
		connectionChr.phyModePtr = phyModeMapper->getBestPhyMode(sinr);
	}
*/
	return apcResult; // TODO
}

