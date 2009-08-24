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

#include <WNS/scheduler/strategy/apcstrategy/FairSINR.hpp>
#include <WNS/scheduler/strategy/apcstrategy/APCStrategy.hpp>
#include <WNS/scheduler/strategy/StrategyInterface.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>
#include <vector>
#include <iostream>
#include <algorithm>

using namespace wns::scheduler;
using namespace wns::scheduler::strategy;
using namespace wns::scheduler::strategy::apcstrategy;

STATIC_FACTORY_REGISTER_WITH_CREATOR(FairSINR,
									 APCStrategyInterface,
									 "FairSINR",
									 wns::PyConfigViewCreator);

FairSINR::FairSINR(const wns::pyconfig::View& config)
	: APCStrategy(config),
	  fair_sinr(wns::Ratio()),
	  fair_sinrdl(config.get<double>("fair_sinrdl")),
	  fair_sinrul(config.get<double>("fair_sinrul"))
{
}

FairSINR::~FairSINR()
{
}

// called before each timeSlot/frame
void
FairSINR::initialize(SchedulerStatePtr schedulerState,
					 SchedulingMapPtr schedulingMap)
{
	APCStrategy::initialize(schedulerState,schedulingMap); // must always initialize base class too
	MESSAGE_SINGLE(NORMAL, logger, "FairSINR::initialize("<<apcstrategyName<<")");
} // initialize


APCResult
FairSINR::doStartAPC(RequestForResource& request,
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
		if (schedulerState->isTx)
		{
			fair_sinr.set_dB(fair_sinrdl);
		}
		else
		{
			fair_sinr.set_dB(fair_sinrul);
		}
		wns::Power fairTxPower = wns::Power::from_mW(fair_sinr.get_factor() * pathloss.get_factor() * interference.get_mW());
		wns::Power totalPower = powerCapabilities.maxOverall;
		wns::Power remainingTxPowerOnAllSubChannels = schedulingMap->getRemainingPower(totalPower);

		if ( remainingTxPowerOnAllSubChannels == wns::Power())
		{
			apcResult.txPower = wns::Power();
			return apcResult;
		}

		if (fairTxPower > powerCapabilities.maxPerSubband)
		{
			fairTxPower = powerCapabilities.maxPerSubband;

			if (fairTxPower > remainingTxPowerOnAllSubChannels)
			{
				apcResult.txPower = remainingTxPowerOnAllSubChannels;
			}
			else
			{
				apcResult.txPower = fairTxPower;
			}
			apcResult.sinr =  apcResult.txPower/(interference * pathloss);
		}
		else
		{
			if (fairTxPower > remainingTxPowerOnAllSubChannels)
			{
				apcResult.txPower = remainingTxPowerOnAllSubChannels;
				apcResult.sinr =  apcResult.txPower/(interference * pathloss);
			}
			else
			{
				apcResult.txPower = fairTxPower;
				apcResult.sinr = fair_sinr;
			}
		}
		apcResult.estimatedCandI = wns::CandI(apcResult.txPower/pathloss,interference);

		apcResult.phyModePtr = phyModeMapper->getBestPhyMode(apcResult.sinr);

	}

	MESSAGE_SINGLE(NORMAL, logger,"doStartAPC("<<request.toString()<<"): "
				   <<"SINR="<<apcResult.sinr<<", PhyMode="<<*(apcResult.phyModePtr));

	/*
	wns::Power maxPowerPerSubChannel = powerCapabilities.maxPerSubband;
	wns::Power maxSummedPowerOnAllChannels = powerCapabilities.maxOverall;
	wns::Power nominalPowerPerSubChannel = powerCapabilities.nominalPerSubband;

	if (schedulingPar.isTx)
	{
		fair_sinr.set_dB(fair_sinrdl);
	}
	else
	{
		fair_sinr.set_dB(fair_sinrul);
	}

	if(schedulingPar.channelQualities.size() > 0)
	{
		wns::Power fairTxPower = wns::Power::from_mW(fair_sinr.get_factor() * schedulingPar.channelQualityOnBestChannel.Pathloss.get_factor() * schedulingPar.channelQualityOnBestChannel.Interference.get_mW());
		wns::Power remainingTxPowerOnAllChannels = colleagues.registry->getRemainingTxPower(schedulingMap,schedulingPar);

		if ( remainingTxPowerOnAllChannels == wns::Power())
		{
			connectionChr.txPower = wns::Power();
			return;
		}

		if (fairTxPower > maxPowerPerSubChannel)
		{
			fairTxPower = maxPowerPerSubChannel;

			if (fairTxPower > remainingTxPowerOnAllChannels)
			{
				connectionChr.txPower = remainingTxPowerOnAllChannels;
			}
			else
			{
				connectionChr.txPower = fairTxPower;
			}
			connectionChr.sinr =  connectionChr.txPower/(schedulingPar.channelQualityOnBestChannel.Interference * schedulingPar.channelQualityOnBestChannel.Pathloss.get_factor());
		}
		else
		{
			if (fairTxPower > remainingTxPowerOnAllChannels)
			{
				connectionChr.txPower = remainingTxPowerOnAllChannels;
				connectionChr.sinr =  connectionChr.txPower/(schedulingPar.channelQualityOnBestChannel.Interference * schedulingPar.channelQualityOnBestChannel.Pathloss.get_factor());
			}
			else
			{
				connectionChr.txPower = fairTxPower;
				connectionChr.sinr = fair_sinr;
			}
		}
		//if (connectionChr.sinr.get_dB() < phyModeMapper->getMinimumSINR())
		//{
		//	connectionChr.txPower = wns::Power();
		//	return;
		//}
		connectionChr.phyModePtr = phyModeMapper->getBestPhyMode(connectionChr.sinr);
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

