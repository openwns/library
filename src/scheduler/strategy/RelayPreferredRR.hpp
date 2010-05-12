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

#ifndef WNS_SCHEDULER_STRATEGY_RELAYPREFERREDROUNDROBIN_HPP
#define WNS_SCHEDULER_STRATEGY_RELAYPREFERREDROUNDROBIN_HPP

#include <WNS/scheduler/strategy/Strategy.hpp>

namespace wns { namespace scheduler { namespace strategy {
	class RelayPreferredRR
	  : public Strategy,
	    public DLAspect,
	    public ULAspect
	{
	public:
		RelayPreferredRR();
		RelayPreferredRR(const wns::pyconfig::View& config)
			: Strategy(config)
			{
				nRemoteStations_ = config.get<int>("nRemoteStations");
				// TODO: get this info via RegistryProxy
				// e.g. from AssociationsProxy in WinProSt
				// [rs,aoz] 26.03.2008
			}

	private:
		virtual void
		doStartScheduling(int fChannels, int maxSpatialLayers, simTimeType slotLength);

		/// Number of remote stations behind a relay station;
		int nRemoteStations_;

		bool
		isGroupRelayGroup( const Group& group );
	};


}}} // namespace wns::scheduler::strategy
#endif


