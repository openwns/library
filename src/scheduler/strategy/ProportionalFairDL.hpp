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

#ifndef WNS_SCHEDULER_STRATEGY_PROPORTIONALFAIRDL_HPP
#define WNS_SCHEDULER_STRATEGY_PROPORTIONALFAIRDL_HPP

#include <WNS/scheduler/strategy/ProportionalFairBase.hpp>

namespace wns { namespace scheduler { namespace strategy {

	class CallBackInterface;

	/**
	 * @brief Proportional Fair scheduler
	 *
	 */

	class ProportionalFairDL
	  : public ProportionalFairBase,
	    public DLAspect,
	    public SlaveAspect
	{
	public:
		~ProportionalFairDL(){}
		ProportionalFairDL(const wns::pyconfig::View& config);

		virtual float
		getResourceUsage() const { return resourceUsage; }

	private:
		virtual void doStartScheduling(int fChannels, int maxSpatialLayers, simTimeType slotLength);

		simTimeType
		scheduleOneBurst(simTimeType burstStart, simTimeType burstLength, Group group, int subBand,
				 std::map<UserID, wns::service::phy::ofdma::PatternPtr> patterns,
				 bool allowForever, wns::Power txPowerPerStream);

		bool oneBurstSucceeded;
		UserSet allUsers;
		double resourceUsage;
	};


}}} // namespace wns::scheduler::strategy

#endif // WNS_SCHEDULER_STRATEGY_PROPORTIONALFAIRDL_HPP


