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

#ifndef WNS_LDK_HASRECEPTOR_HPP
#define WNS_LDK_HASRECEPTOR_HPP

#include <WNS/ldk/HasReceptorInterface.hpp>
#include <WNS/ldk/SingleReceptor.hpp>

namespace wns { namespace ldk {

	/**
	 * @brief HasReceptor specific type declarations.
	 *
	 */
	namespace hasupper
	{
		/**
		 * @brief Default strategy to be used for upper scheduling.
		 *
		 */
		typedef SingleReceptor DefaultReceptorStrategy;
	}

	/**
	 * @brief Implement the HasReceptor Interface for a given scheduling strategy.
	 * @ingroup hasupper
	 *
	 */
	template <typename T = hasupper::DefaultReceptorStrategy>
	class HasReceptor :
		public virtual HasReceptorInterface
	{
	public:
		typedef T ReceptorType;

		HasReceptor() :
			upper()
		{}

		virtual
		~HasReceptor()
		{}

		virtual T*
		getReceptor() const
		{
			return &upper;
		}
	private:
		mutable T upper;
	};
} // ldk
} // wns


#endif // NOT defined WNS_LDK_HASRECEPTOR_HPP



