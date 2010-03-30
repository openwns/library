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

#ifndef WNS_SCHEDULER_STRATEGY_PROPORTIONALFAIRBASE_HPP
#define WNS_SCHEDULER_STRATEGY_PROPORTIONALFAIRBASE_HPP

#include <WNS/scheduler/strategy/Strategy.hpp>
#include <WNS/scheduler/queue/QueueInterface.hpp>
#include <WNS/scheduler/grouper/SpatialGrouper.hpp>
#include <WNS/scheduler/RegistryProxyInterface.hpp>
#include <WNS/distribution/Uniform.hpp>

#include <WNS/StaticFactory.hpp>

namespace wns { namespace scheduler { namespace strategy {

	typedef float DataRate;
	const int noCID     = -1; // TODO: make obsolete

	class CallBackInterface;

	/**
	 * @brief Proportional Fair scheduler
	 *
	 */

	class ProportionalFairBase :
		public Strategy {
	public:
		~ProportionalFairBase();
		ProportionalFairBase(const wns::pyconfig::View& config);
		virtual void onColleaguesKnown();
		typedef std::pair<float, Group> GroupPrefPair; // the float is a preference weight
	protected:
		virtual void
		doStartScheduling(int fChannels, int maxSpatialLayers, simTimeType slotLength) = 0;

		/** @brief preference=f(minRate,groupRate) */
		float
		getPreference(Group group) const;

		/** @brief update pastDataRates=90%*pastDataRates + 10%*currentRate=(dataThisFrame/phaseLength) */
		void
		updatePastDataRates(UserID user, Bits dataThisFrame, simTimeType phaseLength);

		/**@brief indicates that finishedUsers map is empty */
		bool
		everybodyFinished(std::map<UserID, bool> finishedUsers) const;

		/**@brief find the UserID which is to schedule next */
		UserID
		getNextUnfinished(std::map<UserID, simTimeType> timeMarkers, std::map<UserID, bool> finishedUsers) const;

		/**@brief ConnectionID with nonempty queue */
		ConnectionID
		getNextConnWithDataForUser(UserID user) const;

		/**
		 * @brief returns true if at least one of the users in the group has no more compounds to send
		 */
		bool
		groupExhausted(Group group) const;

		virtual simTimeType
		scheduleOneBurst(simTimeType burstStart, simTimeType burstLength, Group group, int subBand,
				 std::map<UserID, wns::service::phy::ofdma::PatternPtr> patterns,
				 bool allowForever, wns::Power txPowerPerStream) = 0;

		/** @brief exponential window sliding average; p_i=(1-h)*c_i + h*i_{i-1} */
		float historyWeight; // Python parameter
		unsigned int maxBursts; // Python parameter
		bool allowReGrouping;
		float scalingBetweenMaxTPandPFair; // 0.0=MaxThroughput; 1.0=ProportionalFair
		float minRateOfSubchannel; // rate per subchannel for lowest PhyMode
		float maxRateOfSubchannel; // rate per subchannel for highest PhyMode
		std::map<UserID, Bits> bitsThisFrame;
		CallBackInterface* parent;
		std::map<UserID, DataRate> pastDataRates;
		/**
		 * @brief distribution for random numbers used to variate the
		 preference a little bit.
		 */
		wns::distribution::Uniform* preferenceVariationDistribution;
	};


}}} // namespace wns::scheduler::strategy
namespace std {
 	template <>
 	struct less<wns::scheduler::strategy::ProportionalFairBase::GroupPrefPair>:
	public binary_function<wns::scheduler::strategy::ProportionalFairBase::GroupPrefPair,
			       wns::scheduler::strategy::ProportionalFairBase::GroupPrefPair,
			       bool>{
 		bool operator()(const wns::scheduler::strategy::ProportionalFairBase::GroupPrefPair & lhs,
				const wns::scheduler::strategy::ProportionalFairBase::GroupPrefPair & rhs) const
 		{
			if(lhs.first == rhs.first) {
				//UserID of the first User of a group is taken if floats(preferences) are equal
				return lhs.second.begin()->first->getName() < rhs.second.begin()->first->getName();
			}
			return lhs.first < rhs.first;
 		}
 	};
}
#endif // WNS_SCHEDULER_STRATEGY_PROPORTIONALFAIRBASE_HPP


