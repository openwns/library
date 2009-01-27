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

#ifndef WNS_LDK_COMPOUNDHANDLERINTERFACE_HPP
#define WNS_LDK_COMPOUNDHANDLERINTERFACE_HPP

#include <WNS/ldk/Compound.hpp>

namespace wns { namespace ldk {

	class Layer;

	/**
	 * @defgroup compoundhandler Compound Handler Interface
	 * @ingroup ldkaspects
	 * @brief Deal with compounds (inject, drop, mutate) and realise intra layer flow control.
	 *
	 * To implement the aspect of dealing with compounds, ldk provides several
	 * alternatives. The most basic way is to implement the CompoundHandlerInterface directly.
	 * The CompoundHandlerInterface requests you to implement a set of 4 methods. First,
	 * @li CompoundHandlerInterface::sendData
	 * @li CompoundHandlerInterface::onData
	 * for Compound delivery, second
	 * @li CompoundHandlerInterface::doIsAccepting
	 * @li CompoundHandlerInterface::doWakeup
	 * for intra-layer flow control.
	 * <br>
	 * But there exist some adapters for making writing functional units more convenient:
	 * @li Processor - for functional units without internal memory.
	 * @li Delayed - for functional units with internal memory, but without Compound-sensitive
	 *     acceptance.
	 */

	/**
	 * @brief Interface for the aspect of dealing with compounds.
	 * @ingroup compoundhandler
	 */
	struct CompoundHandlerInterface
	{
		/**
		 * @brief Return true if you are willing to accept the given Compound.
		 *
		 * Functional units without internal memory delegate the isAccepting query to
		 * one of their lower functional units. On success, this results in a path through
		 * the layer, where each FunctionalUnit in the path promised to one of its
		 * upper functional units to accept the given Compound.
		 * <br>
		 * Each of the functional units in the path has to assure to deliver the Compound
		 * to the FunctionalUnit he asked using isAccepting. If it does not, chances are,
		 * that a Compound reaches a FunctionalUnit that has no internal storage and cannot
		 * be delivered further. That is an illegal state and can only be avoided
		 * if functional units conform to the above rule.
		 * <br>
		 * When paths of promises are created by recursively calling
		 * isAccepting, a FunctionalUnit may get asked to decide, whether it accepts a
		 * given Compound although the Commands that it builds its decision on, have
		 * not yet been activated. That, too, is illegal behaviour and can only
		 * be avoided by the careful placement of buffers. A buffer implementation
		 * for exactly this purpose is the Synchronizer. A FunctionalUnit such as the
		 * Synchronizer accepts compounds regardless of their content. This breaks chains
		 * of recursive isAccepting calls. That's fine
		 */
		bool
		isAccepting(const CompoundPtr& compound)
		{
			return isAcceptingForwarded(compound);
		}

		/**
		 * @brief Accept Compound from a higher FunctionalUnit.
		 *
		 * A higher FunctionalUnit delivers a Compound using doSendData. The higher FunctionalUnit asked
		 * for acceptance using isAccepting with the same Compound in advance.
		 * <br>
		 * It is the job of each FunctionalUnit to implement Compound delivery to lower functional units.
		 * Never call doSendData directly, always make shure that the FunctionalUnit is
		 * accepting the given Compound by calling isAccepting in advance.
		 */
		void
		sendData(const CompoundPtr& compound)
		{
			assure( isAccepting(compound), "FU does not accept compound");
			sendDataForwarded(compound);
		}

		/**
		 * @brief Your callback, when lower functional units may be accepting again.
		 *
		 * When there is no FunctionalUnit holding a Compound that it may deliver to its lower
		 * functional units, the net of functional units gets inactive. Only external events (other than
		 * defined by the ldk api) may trigger the functional units again:
		 * @li a top FunctionalUnit may receive a fresh Compound from outside the FunctionalUnit stack.
		 * @li a bottom FunctionalUnit may manage to deliver a Compound to the outside and thus change
		 *     state to accept new compounds.
		 * @li any FunctionalUnit may change state through a timeout.
		 * <br>
		 * Whenever a FunctionalUnit changes state from not accepting to accepting, it must
		 * inform its upper layers of the change using the wakeup method.
		 */
		void
		wakeup()
		{
			wakeupForwarded();
		}

		/**
		 * @brief Accept Compound from a lower FunctionalUnit in the incoming data flow.
		 *
		 * A lower FunctionalUnit delivers a Compound using doOnData.
		 * <br>
		 * It is the job of each FunctionalUnit to implement Compound delivery to upper functional units.
		 */
		void
		onData(const CompoundPtr& compound)
		{
			onDataForwarded(compound);
		}

		/**
		 * @brief virtual functions -> virtual d'tor
		 */
		virtual
		~CompoundHandlerInterface()
		{}

	protected:
		/**
		 * @brief This function is used to forward the function calls of
		 * isAccepting(...) to the link handler.
		 *
		 * @note This functions is implemented in class
		 * FunctionalUnit. Never overload it in other derived
		 * classes!
		 */
		virtual bool
		isAcceptingForwarded(const CompoundPtr& compound) = 0;

		/**
		 * @brief This function is used to forward the function calls of
		 * sendData(...) to the link handler.
		 *
		 * @note This functions is implemented in class
		 * FunctionalUnit. Never overload it in other derived
		 * classes!
		 */
		virtual void
		sendDataForwarded(const CompoundPtr& compound) = 0;

		/**
		 * @brief This function is used to forward the function calls of
		 * wakeup() to the link handler.
		 *
		 * @note This functions is implemented in class
		 * FunctionalUnit. Never overload it in other derived
		 * classes!
		 */
		virtual void
		wakeupForwarded() = 0;

		/**
		 * @brief This function is used to forward the function calls of
		 * onData(...) to the link handler.
		 *
		 * @note This functions is implemented in class
		 * FunctionalUnit. Never overload it in other derived
		 * classes!
		 */
		virtual void
		onDataForwarded(const CompoundPtr& compound) = 0;

	private:

		friend class LinkHandlerInterface;

		/**
		 * @brief Forwarded from isAccepting()
		 *
		 * To specifiy the behaviour of the isAccepting() method you have to
		 * overload this method (part of NVI)
		 */
		virtual bool
		doIsAccepting(const CompoundPtr& compound) const = 0;

		/**
		 * @brief Forwarded from sendData()
		 *
		 * To specifiy the behaviour of the sendData() method you have to
		 * overload this method (part of NVI)
		 */
		virtual void
		doSendData(const CompoundPtr& compound) = 0;

		/**
		 * @brief Forwarded from wakeup()
		 *
		 * To specifiy the behaviour of the wakeup() method you have to overload
		 * this method (part of NVI)
		 */
		virtual void
		doWakeup() = 0;

		/**
		 * @brief Forwarded from onData()
		 *
		 * To specifiy the behaviour of the onData() method you have to overload
		 * this method (part of NVI)
		 */
		virtual void
		doOnData(const CompoundPtr& compound) = 0;

	};

} // ldk
} // wns


#endif // NOT defined WNS_LDK_COMPOUNDHANDLERINTERFACE_HPP


