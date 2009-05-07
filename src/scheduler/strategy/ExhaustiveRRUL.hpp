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

#ifndef WNS_SCHEDULER_STRATEGY_EXHAUSTIVERR_HPP
#define WNS_SCHEDULER_STRATEGY_EXHAUSTIVERR_HPP

#include <WNS/scheduler/strategy/Strategy.hpp>

namespace wns { namespace scheduler { namespace strategy {
	class ExhaustiveRRUL
	  : public Strategy,
	    public ULAspect,
	    public SlaveAspect
	{
	public:
		ExhaustiveRRUL(const wns::pyconfig::View& config)
		  : Strategy(config),
		    unservedUsersLastRound(false),
		    resourceUsage(0.0)
			{}
		~ExhaustiveRRUL() {}

		virtual float
		getResourceUsage() const { return resourceUsage; }

	private:
		virtual void
		doStartScheduling(int fChannels, int maxBeams, simTimeType slotLength);
		bool
		everyGroupMemberWasServed(Group group);

		std::set<UserID> usersServedLastRound;
		bool unservedUsersLastRound;
		double resourceUsage;
	};


}}} // namespace wns::scheduler::strategy
#endif // WNS_SCHEDULER_STRATEGY_EXHAUSTIVERR_HPP


