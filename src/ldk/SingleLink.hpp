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

#ifndef WNS_LDK_SINGLELINK_HPP
#define WNS_LDK_SINGLELINK_HPP

#include <WNS/ldk/Link.hpp>

namespace wns { namespace ldk {

	class SingleLink :
		virtual public Link
	{
	public:
		SingleLink();

		virtual
		~SingleLink();

		/**
		 * @name Link interface
		 */
		//{@
		virtual void
		add(FunctionalUnit* it);

		virtual uint32_t
		size() const;

		virtual void
		clear();

		virtual const Link::ExchangeContainer
		get() const;

		virtual void
		set(const Link::ExchangeContainer&);
		//@}

	protected:
		FunctionalUnit*
		getFU() const
		{
			return fu;
		}

	private:
		FunctionalUnit* fu;
	};
} // ldk
} // wns

#endif // WNS_LDK_SINGLELINK_HPP


