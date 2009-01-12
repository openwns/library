/*******************************************************************************
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2004-2007
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

#ifndef WNS_LDK_LINKHANDLERINTERFACE_HPP
#define WNS_LDK_LINKHANDLERINTERFACE_HPP

#include <WNS/ldk/Compound.hpp>
#include <WNS/ldk/FunctionalUnit.hpp>

namespace wns { namespace ldk {

	class LinkHandlerInterface
	{
	public:
		virtual bool
		isAcceptingForwarded(FunctionalUnit* fu, const CompoundPtr& compound) = 0;

		virtual void
		sendDataForwarded(FunctionalUnit* fu, const CompoundPtr& compound) = 0;

		virtual void
		wakeupForwarded(FunctionalUnit* fu) = 0;

		virtual void
		onDataForwarded(FunctionalUnit* fu, const CompoundPtr& compound) = 0;

		virtual
		~LinkHandlerInterface()
		{}

	protected:
		virtual bool
		doIsAccepting(FunctionalUnit* fu, const CompoundPtr& compound)
		{
			return fu->doIsAccepting(compound);
		}

		virtual void
		doSendData(FunctionalUnit* fu, const CompoundPtr& compound)
		{
			fu->doSendData(compound);
		}

		virtual void
		doWakeup(FunctionalUnit* fu)
		{
			fu->doWakeup();
		}

		virtual void
		doOnData(FunctionalUnit* fu, const CompoundPtr& compound)
		{
			fu->doOnData(compound);
		}
	};

} // ldk
} // wns

#endif // NOT defined WNS_LDK_LINKHANDLERINTERFACE_HPP


