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

#ifndef WNS_SCHEDULER_STRATEGY_CQIENABLEDEXHAUSTIVERR_HPP
#define WNS_SCHEDULER_STRATEGY_CQIENABLEDEXHAUSTIVERR_HPP

#include <WNS/scheduler/strategy/Strategy.hpp>
#include <vector>
#include <map>
namespace wns { namespace scheduler { namespace strategy {

	typedef float DataRate;

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

	// CQI Enhanced Tx strategy used in AP, RN-BS (for master sheduling), can not be used in UT!!!
	class CQIEnabledExhaustiveRR
	  : public Strategy,
	    public DLAspect
	{
	public:
		CQIEnabledExhaustiveRR(const wns::pyconfig::View& config)
			: Strategy(config),
			unservedUsersLastRound(false),
			useNominalTxPower(config.get<bool>("useNominalTxPower")),
			useRandomChannelAtBeginning(config.get<bool>("useRandomChannelAtBeginning")),
			maxPowerPerSubChannel(),
			maxSummedPowerOnAllChannels(),
			nominalPowerPerSubChannel(),
			bestChannel(0)
			{}
		~CQIEnabledExhaustiveRR() {}

		virtual void onColleaguesKnown();

		/** @brief get the remaining tx power on all the channels */
		wns::Power
		getRemainedTxPower(int channel, double beginTime, wns::Power maxSummedPowerOnAllChannels, usedTxPowerOnAllChannels& usedTxPower);

	private:
		virtual void
		doStartScheduling(int fChannels, int maxSpatialLayers, simTimeType slotLength);
		bool
		everyGroupMemberWasServed(Group group);

		// get the best channel for user,
		// the best channel means the channel which has the biggest capcity for user(restTime * phyMode.dataRate).
		ChannelQualityOnOneSubChannel
		getBestChannel(ChannelQualitiesOnAllSubBandsPtr channelQualities,
			       std::vector<double>& nextFreeSlot,
			       double slotLength,
			       usedTxPowerOnAllChannels& usedTxPower);

		std::set<UserID> usersServedLastRound;
		bool unservedUsersLastRound;

		/** @brief the phyModeMapper can calculate PhyMode from SINR and back */
		wns::service::phy::phymode::PhyModeMapperInterface* phyModeMapper;

		//if it is true, it means we decide to use the same txPower as the old strategy(see ExhaustiveRR ),so
		//there is no adaptive Txpower control here.
                bool useNominalTxPower;

		//if it is true, it means we decide to randomly assign channels to user if we have no channel qualities of it
                bool useRandomChannelAtBeginning;

		wns::Power maxPowerPerSubChannel ;
		wns::Power maxSummedPowerOnAllChannels ;
		wns::Power nominalPowerPerSubChannel ;

		int bestChannel;
	};


}}} // namespace wns::scheduler::strategy
#endif // WNS_SCHEDULER_STRATEGY_EXHAUSTIVERR_HPP


