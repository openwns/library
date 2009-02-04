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

#ifndef WNS_LDK_TOOLS_DOWNUNCONNECTABLE_HPP
#define WNS_LDK_TOOLS_DOWNUNCONNECTABLE_HPP

#include <sstream>

#include <WNS/ldk/FunctionalUnit.hpp>

namespace wns { namespace ldk { namespace tools {

	class DownUnconnectable :
		public virtual FunctionalUnit,
		public virtual CompoundHandlerInterface,
		public virtual HasConnectorInterface,
		public virtual HasReceptorInterface
	{
		virtual void _connect(FunctionalUnit*)
		{
			std::ostringstream ss;
			ss << "You cannot connect the Functional Unit " << this->getName();
			throw wns::Exception(ss.str());
		}

		virtual void _downConnect(FunctionalUnit*)
		{
			std::ostringstream ss;
			ss << "You cannot connect the Functional Unit " << this->getName();
			throw wns::Exception(ss.str());
		}

		virtual void _upConnect(FunctionalUnit*)
		{
			std::ostringstream ss;
			ss << "You cannot connect the Functional Unit " << this->getName();
			throw wns::Exception(ss.str());
		}

		virtual Connector*
		getConnector() const
		{
			std::ostringstream ss;
			ss << "Impossible to get the Connector of the Functional Unit" << this->getName();
			throw wns::Exception(ss.str());
		}

		virtual Receptor*
		getReceptor() const
		{
			std::ostringstream ss;
			ss << "Impossible to get the Receptor of the Functional Unit" << this->getName();
			throw wns::Exception(ss.str());
		}

		virtual void
		doWakeup()
		{
			std::ostringstream ss;
			ss << "Impossible to call wakeup of the Functional Unit" << this->getName();
			throw wns::Exception(ss.str());
		}

		virtual void
		doOnData(const CompoundPtr&)
		{
			std::ostringstream ss;
			ss << "Impossible to call onData of the Functional Unit" << this->getName();
			throw wns::Exception(ss.str());
		}
}}}
#endif


