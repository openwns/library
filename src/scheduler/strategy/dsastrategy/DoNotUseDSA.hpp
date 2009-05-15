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

#ifndef WNS_SCHEDULER_DSASTRATEGY_DONOTUSEDSA_HPP
#define WNS_SCHEDULER_DSASTRATEGY_DONOTUSEDSA_HPP

#include <WNS/scheduler/strategy/dsastrategy/DSAStrategy.hpp>

namespace wns { namespace scheduler { namespace strategy { namespace dsastrategy {

	// Do not use DSA strategy
	class DoNotUseDSA :
		public DSAStrategy {
	public:
		DoNotUseDSA(const wns::pyconfig::View& config);

		~DoNotUseDSA() {};

		/** @brief determine subChannel for request.
		    This is only a suggestion. The allocation/fixation is done outside. */
		virtual DSAResult
		getSubChannelWithDSA(RequestForResource& request,
				     SchedulerStatePtr schedulerState,
				     SchedulingMapPtr schedulingMap);

		bool requiresCQI() const { return false; };
	};
}}}} // namespace wns::scheduler::strategy::dsastrategy
#endif // WNS_SCHEDULER_DSASTRATEGY_DONOTUSEDSA_HPP

/*
  Local Variables:
  mode: c++
  fill-column: 80
  c-basic-offset: 8
  c-comment-only-line-offset: 0
  c-tab-always-indent: t
  indent-tabs-mode: t
  tab-width: 8
  End:
*/