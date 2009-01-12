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

#ifndef WNS_LDK_TOOLS_FAKEFU_HPP
#define WNS_LDK_TOOLS_FAKEFU_HPP

#include <WNS/ldk/HasReceptor.hpp>
#include <WNS/ldk/HasConnector.hpp>
#include <WNS/ldk/HasDeliverer.hpp>
#include <WNS/ldk/Forwarding.hpp>
#include <WNS/ldk/fun/FUN.hpp>


namespace wns { namespace ldk { namespace tools {

	/**
	 * @brief In case you need an FU for testing without FUN
	 * @author Marc Schinnenburg <msg@comnets.rwth-aachen.de>
	 *
	 * This FU is intentionally not registered at the StaticFactory
	 */
	class FakeFU :
		public HasReceptor<>,
		public HasConnector<>,
		public HasDeliverer<>,
		public Forwarding<FakeFU>,
		public Cloneable<FakeFU>
	{
	public:
		FakeFU() :
			HasReceptor<>(),
			HasConnector<>(),
			HasDeliverer<>(),
			Forwarding<FakeFU>(),
			Cloneable<FakeFU>()
		{}

		virtual fun::FUN*
		getFUN() const
		{
			return NULL;
		}

		virtual wns::ldk::Command*
		getCommand(const wns::ldk::CommandPool*) const
		{
			return NULL;
		}

		virtual wns::ldk::Command*
		activateCommand(wns::ldk::CommandPool*) const
		{
			return NULL;
		}

		virtual wns::ldk::CommandPool*
		createReply(const wns::ldk::CommandPool*) const
		{
			return NULL;
		}

		virtual void
		calculateSizes(const wns::ldk::CommandPool*, Bit&, Bit&) const
		{
			return;
		}

		virtual void
		commitSizes(wns::ldk::CommandPool*) const
		{
			return;
		}

		virtual wns::ldk::Command*
		createCommand() const
		{
			return NULL;
		}

		virtual wns::ldk::Command*
		copyCommand(const wns::ldk::Command*) const
		{
			return NULL;
		}

		virtual wns::ldk::CopyCommandInterface*
		getCopyCommandInterface() const
		{
			return NULL;
		}

		virtual CommandReaderInterface*
		getCommandReader(const CommandProxy*) const
		{
			return NULL;
		}

#ifndef NDEBUG
		virtual size_t
		getCommandObjSize() const
		{
			return 0;
		}
#endif
	};

}
}
}

#endif // NOT defined WNS_LDK_TOOLS_FAKEFU_HPP



