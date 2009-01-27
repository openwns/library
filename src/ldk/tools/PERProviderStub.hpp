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

#ifndef WNS_LDK_TOOLS_PERPROVIDERSTUB_HPP
#define WNS_LDK_TOOLS_PERPROVIDERSTUB_HPP

#include <WNS/ldk/tools/Stub.hpp>
#include <WNS/ldk/CommandTypeSpecifier.hpp>
#include <WNS/ldk/HasReceptor.hpp>
#include <WNS/ldk/HasConnector.hpp>
#include <WNS/ldk/HasDeliverer.hpp>
#include <WNS/ldk/ErrorRateProviderInterface.hpp> 

#include <WNS/Cloneable.hpp>
#include <WNS/pyconfig/View.hpp>

#include <deque>


namespace wns { namespace ldk { namespace tools {

	/**
	 * @brief Provide a fixed PER for use by other layers
	 *
	 */
	class PERProviderPCI :
		public EmptyCommand,
		public ErrorRateProviderInterface
	{
	public:
		PERProviderPCI()
			: PER(0.0)
			{};
		void
		setPER(double _PER){ PER = _PER; };

		virtual double
		getErrorRate() const { return PER; }
	private:
		double PER;
	}; // PERProviderPCI

	class PERProviderStub :
		public StubBase,
		public CommandTypeSpecifier<PERProviderPCI>,
		public Cloneable<PERProviderStub>
	{
	public:
		/**
		 * Set stepping to true, if you whish to have a PERProviderStub with
		 * debugger like stepping of execution.
		 */
		PERProviderStub(fun::FUN* fuNet, const pyconfig::View& config);

		// we need a unique overrider
		virtual void
		calculateSizes(const CommandPool* commandPool, Bit& commandPoolSize, Bit& dataSize) const
		{
			StubBase::calculateSizes(commandPool, commandPoolSize, dataSize);
		} // calculateSizes

	private:
		virtual void
		doOnData(const CompoundPtr& compound);

		double fixedPER;
	}; // PERProviderStub

}}}


#endif // NOT defined WNS_LDK_TOOLS_PERPROVIDERSTUB_HPP


