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

#ifndef WNS_SCHEDULER_STRATEGY_PCRR_HPP
#define WNS_SCHEDULER_STRATEGY_PCRR_HPP

#include <WNS/scheduler/strategy/Strategy.hpp>
#include <WNS/scheduler/queue/QueueInterface.hpp>
#include <WNS/scheduler/grouper/SpatialGrouper.hpp>
#include <WNS/scheduler/RegistryProxyInterface.hpp>

#include <WNS/StaticFactory.hpp>

namespace wns { namespace scheduler { namespace strategy {

	class CallBackInterface;

	struct GroupAlloc {
		uint32_t group;
		uint32_t blocks;
	};

	bool
	LtAlloc(const GroupAlloc& a1, const GroupAlloc& a2)
	{
		return (a1.blocks < a2.blocks);
	}

	uint32_t
	countSumBlocks(const std::list<GroupAlloc>& aList)
	{
		uint32_t result = 0;
		for (std::list<GroupAlloc>::const_iterator iter = aList.begin();
		     iter != aList.end();
		     ++iter)
		{
			result += iter->blocks;
		}
		return result;
	}

	/**
	 * @brief Base class for all RR schedulers. Provides an equal time
	 * UL-scheduling.
	 *
	 */
	class PCRR
	  : public Strategy,
	    public ULAspect
	{
	public:
		~PCRR() {}
		PCRR(const wns::pyconfig::View& config) :
			Strategy(config),
			blockDuration(config.get<simTimeType>("blockDuration")),
			resourceUsage(0.0)
		{}

		virtual float
		getResourceUsage() const { return resourceUsage; }

	private:

		virtual void
		doStartScheduling(int fChannels, int maxBeams, simTimeType slotLength);

		simTimeType blockDuration;
		float resourceUsage;
	};


}}} // namespace wns::scheduler::strategy
#endif // WNS_SCHEDULER_STRATEGY_ROUNDROBIN_HPP


