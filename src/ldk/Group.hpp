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

#ifndef WNS_LDK_GROUP_HPP
#define WNS_LDK_GROUP_HPP

#include <WNS/ldk/CommandTypeSpecifier.hpp>
#include <WNS/ldk/HasReceptor.hpp>
#include <WNS/ldk/HasConnector.hpp>
#include <WNS/ldk/HasDeliverer.hpp>

#include <WNS/ldk/fun/Sub.hpp>

#include <WNS/logger/Logger.hpp>

#include <WNS/pyconfig/View.hpp>

namespace wns { namespace ldk {

	/**
	 * @defgroup ldkgroup Group
	 * @ingroup ldk
	 *
	 * @brief Build groups of FUs acting as a single FU.
	 *
	 */

	namespace group
	{
		/**
		 * @brief Redirect Sub FUN requests to the Group's receptor/deliverer.
		 * @ingroup ldkgroup
		 *
		 * This FU is part of the Group FU. You don't have to understand this,
		 * unless you want to learn about the internals of Group.
		 *
		 * This FU gets connected to the /top/ FU of the Sub FUN to redirect
		 * requests from the top FU to higher FUs. Thus, doOnData and wakeup are
		 * the only methods allowed to call.
		 *
		 * see wns::ldk::Group for a discussion
		 */
		class TopRedirector :
			public virtual FunctionalUnit,
			public CommandTypeSpecifier<>,
			public HasReceptor<>,
			public HasConnector<>,
			public HasDeliverer<>,
			public Cloneable<TopRedirector>

		{
		public:
			TopRedirector(FunctionalUnit* _idol) :
					CommandTypeSpecifier<>(_idol->getFUN()),
					idol(_idol)
			{}

			virtual void onFUNCreated()
			{
				abort();
			} // onFUNCreated



			virtual void
			doSendData(const CompoundPtr& /* compound */)
			{
				abort();
			} // doSendData


			virtual void
			doOnData(const CompoundPtr& compound)
			{
				idol->getDeliverer()->getAcceptor(compound)->onData(compound);
			} // doOnData


		private:
			virtual bool
			doIsAccepting(const CompoundPtr& /* compound */) const
			{
				abort();
			} // isAccepting

			virtual void
			doWakeup()
			{
				idol->getReceptor()->wakeup();
			} // wakeup

			FunctionalUnit* idol;
		};


		/**
		 * @brief Redirect Sub FUN requests to the Group's connector.
		 * @ingroup ldkgroup
		 *
		 * This FU is part of the Group FU. You don't have to understand this,
		 * unless you want to learn about the internals of Group.
		 *
		 * This FU gets connected to the /bottom/ FU of the Sub FUN to redirect
		 * requests from the bottom FU to lower FUs. Thus, doSendData and isAccepting
		 * are the only methods allowed to call.
		 *
		 * See wns::ldk::Group for a discussion.
		 */
		class BottomRedirector :
			public virtual FunctionalUnit,
			public CommandTypeSpecifier<>,
			public HasReceptor<>,
			public HasConnector<>,
			public HasDeliverer<>,
			public Cloneable<BottomRedirector>

		{
		public:
			BottomRedirector(FunctionalUnit* _idol) :
					CommandTypeSpecifier<>(_idol->getFUN()),
					idol(_idol)
			{}

			virtual void onFUNCreated()
			{
				abort();
			} // onFUNCreated


			virtual void
			doSendData(const CompoundPtr& compound)
			{
				return idol->getConnector()->getAcceptor(compound)->sendData(compound);
			} // doSendData


			virtual void
			doOnData(const CompoundPtr& /* compound */)
			{
				abort();
			} // doOnData

		private:
			virtual bool
			doIsAccepting(const CompoundPtr& compound) const
			{
				return idol->getConnector()->hasAcceptor(compound);
			} // isAccepting

			virtual void
			doWakeup()
			{
				abort();
			} // wakeup

			FunctionalUnit* idol;
		};
	}

	/**
	 * @brief Functional Unit holding a Sub FUN.
	 * @ingroup ldkgroup
	 *
	 * ldk::fun::Sub is a FUN that can be placed within another FUN. Group is
	 * a FU wrapping such a Sub FUN.
	 *
	 * Group has two major use cases:
	 *  1. Logical Grouping
	 *  2. Interplay with FlowSeparator
	 *
	 */
	class Group :
		public virtual FunctionalUnit,
		public CommandTypeSpecifier<>,
		public HasReceptor<>,
		public HasConnector<>,
		public HasDeliverer<>,
		public Cloneable<Group>
	{
	public:
		Group(fun::FUN* fuNet, const pyconfig::View& _config);
		~Group();

		Group(const Group& other);

		//
		// FU interface
		//
		virtual void onFUNCreated();
		virtual void doSendData(const CompoundPtr& compound);
		virtual void doOnData(const CompoundPtr& compound);

		virtual void setName(std::string _name);

		fun::Sub* getSubFUN() const;

	private:
		virtual bool doIsAccepting(const CompoundPtr& compound) const;
		virtual void doWakeup();

		pyconfig::View config;
		logger::Logger logger;

		group::TopRedirector topRedirector;
		group::BottomRedirector bottomRedirector;

		fun::Sub* sub;
		FunctionalUnit* topFU;
		FunctionalUnit* bottomFU;
	};

}}


#endif // NOT defined WNS_LDK_GROUP_HPP


