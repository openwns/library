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
#ifndef WNS_SCHEDULER_STRATEGY_CQIENABLEDROUNDROBIN_HPP
#define WNS_SCHEDULER_STRATEGY_CQIENABLEDROUNDROBIN_HPP

#include <WNS/scheduler/strategy/Strategy.hpp>
#include <WNS/scheduler/queue/QueueInterface.hpp>
#include <WNS/scheduler/grouper/SpatialGrouper.hpp>
#include <WNS/scheduler/RegistryProxyInterface.hpp>

#include <WNS/StaticFactory.hpp>

namespace wns { namespace scheduler { namespace strategy {

	class CallBackInterface;

	//class which is used to record the txPower usage
	class TxPower4PDU
	{
	public:
		TxPower4PDU():
			beginTime(0.0),
			endTime(0.0),
			txPower()
			{}

		TxPower4PDU(double beginT, double endT, wns::Power usedPower):
			beginTime(beginT),
			endTime(endT),
			txPower(usedPower)
			{}

		double beginTime;
		double endTime;
		wns::Power txPower;
	};

	typedef std::vector<TxPower4PDU> usedTxPowerOnOneChannel;
	typedef std::vector<usedTxPowerOnOneChannel> usedTxPowerOnAllChannels;

	// CQI Enhanced Rx strategy used in AP, RN-BS (for master sheduling)
	class CQIEnabledRoundRobinUL
	  : public Strategy,
	    public ULAspect
	{
	public:
		~CQIEnabledRoundRobinUL() {}
		CQIEnabledRoundRobinUL(const wns::pyconfig::View& config) :
			Strategy(config),
			blockDuration(config.get<simTimeType>("blockDuration")),
			useNominalTxPower(config.get<bool>("useNominalTxPower"))
		{}

		virtual void onColleaguesKnown();

		/** @brief get the remaining tx power on all the channels */
		wns::Power
		getRemainedTxPower(int channel, double beginTime, wns::Power maxSummedPowerOnAllChannels, usedTxPowerOnAllChannels& usedTxPower);

	private:
		virtual void
		doStartScheduling(int fChannels, int maxBeams, simTimeType slotLength);

		simTimeType blockDuration;

		//if it is true, it means we decide to use the same txPower as the old strategy(see ExhaustiveRR ),so
		//there is no adaptive Txpower control here.
                bool useNominalTxPower;

		wns::service::phy::phymode::PhyModeMapperInterface* phyModeMapper;
	};


}}} // namespace wns::scheduler::strategy
#endif // WNS_SCHEDULER_STRATEGY_ROUNDROBIN_HPP


