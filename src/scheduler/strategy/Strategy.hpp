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

#ifndef WNS_SCHEDULER_STRATEGY_STRATEGY_HPP
#define WNS_SCHEDULER_STRATEGY_STRATEGY_HPP

#include <WNS/scheduler/strategy/StrategyInterface.hpp>
#include <WNS/scheduler/strategy/SchedulerState.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>
#include <WNS/scheduler/queue/QueueInterface.hpp>
#include <WNS/scheduler/grouper/SpatialGrouper.hpp>
#include <WNS/scheduler/strategy/apcstrategy/APCStrategyInterface.hpp>
#include <WNS/scheduler/strategy/dsastrategy/DSAStrategyInterface.hpp>
#include <WNS/service/phy/phymode/PhyModeInterface.hpp>
#include <WNS/logger/Logger.hpp>

namespace wns { namespace scheduler { namespace strategy {

      const simTimeType slotLengthRoundingTolerance = 1e-12;

      class CapabilityAspect
      {
      public:
	virtual ~CapabilityAspect(){};
	virtual bool canHandleDL() { return false; };
	virtual bool canHandleUL() { return false; };
	virtual bool canBeSlave()  { return false; };
      };

      /** @brief let your strategy derive from this if it supports DL scheduling */
      class DLAspect
	: virtual public CapabilityAspect
      {
      public:
	DLAspect(){};
	virtual ~DLAspect(){};
	virtual bool canHandleDL() { return true; };
      };

      /** @brief let your strategy derive from this if it supports UL master scheduling */
      class ULAspect
	: virtual public CapabilityAspect
      {
      public:
	ULAspect(){};
	virtual ~ULAspect(){};
	virtual bool canHandleUL() { return true; };
      };

      /** @brief let your strategy derive from this if it supports UL slave scheduling */
      class SlaveAspect
	: virtual public CapabilityAspect
      {
      public:
	SlaveAspect(){};
	virtual ~SlaveAspect(){};
	virtual bool canBeSlave()  { return true; };
      };

      /** @brief this is the base class of a scheduler strategy;
	  it provides basic operations common for all strategies.
	  See StaticPriority and SubStrategies for more advanced concepts */
	class Strategy
	  : virtual public StrategyInterface,
	    virtual public CapabilityAspect
	{
	public:
		Strategy(const wns::pyconfig::View& config);
		/** @brief cleanup. The derived method MUST call this base class method at the end.  */
		virtual ~Strategy();
		/** @brief Colleagues are required to communicate with the environment */
		virtual void setColleagues(queue::QueueInterface* _queue,
					   grouper::GroupingProviderInterface* _grouper,
					   RegistryProxyInterface* _registry);

		/** @brief can be overloaded by derived classes to perform initialization code.
		    The derived method MUST call this base class method first. */
		virtual void onColleaguesKnown();

		virtual void setFriends(wns::service::phy::ofdma::BFInterface* _ofdmaProvider);

		/** @brief helper method to support the old scheduler interface (for WiMAC). */
		int getNumBursts() const;

		/** @brief retrieve info about a peer user's power capabilities (for master UL scheduling) */
		wns::scheduler::PowerCapabilities
		getPowerCapabilities(const UserID user) const;

		/** @brief obtain percentage of resources allocated during last round.
		    Default Implementation only. */
		virtual float getResourceUsage() const;

		/** @brief performs resource scheduling including DSA, AMC and APC algorithms */
		virtual MapInfoEntryPtr
		doAdaptiveResourceScheduling(wns::scheduler::strategy::RequestForResource& request,
					     //wns::scheduler::SchedulerStatePtr schedulerState,
					     wns::scheduler::SchedulingMapPtr schedulingMap);

		/** @brief indicates type of ResourceScheduler (TX or RX).
		    Only used in WinProSt. */
		virtual bool isTx() const;

		/** @brief indicates if strategy is of 'old' (before 2009) or 'new' type (>=2009). Used in assertions. */
		virtual bool isNewStrategy() const { return false; }

		/** @brief get scheduling result. For sending Maps. Called from MapHandler.
		    The caller must do colleagues.strategy->clearMap() afterwards.
		    Used by WinProSt. */
		MapInfoCollectionPtr getMapInfo() const;

		/** @brief helper method to support the old scheduler strategies. */
		virtual void clearMap();

	protected:

		/** @brief The strategies need a new state. */
		virtual SchedulerStatePtr
		getNewSchedulerState();
		/** @brief The state is revolved before a new doStartScheduling() call. */
		virtual SchedulerStatePtr
		revolveSchedulerState(const StrategyInput& strategyInput);
		/** @brief Get the scheduler state here. */
		virtual SchedulerStatePtr
		getSchedulerState();

		/** @brief (for AP) determines the order of PDUs in the given full set of resources.
		    calls doStartScheduling() of derived class.
		    @param StrategyInput contains parameters set by the caller.
		    @return StrategyResult collection. All set inside.
		*/
		virtual StrategyResult
		startScheduling(const StrategyInput& StrategyInput);

		/** @brief determines how moch power is still available assuming that the total transmit power can be freely distributed onto the subChannels. */
		virtual wns::Power
		getRemainingTxPower(SchedulingMapPtr schedulingMap) const;

		/** @brief return decision of the Link Adaptation algorithm or
		 * the fixed, predefined phymode */
		virtual wns::service::phy::phymode::PhyModeInterfacePtr
		getBestPhyMode(const wns::Ratio& sinr) const;

		virtual bool groupingRequired() const;

		/** @brief call this method after all compounds have been scheduled.
		    Calls the callback() */
		void
		schedulingMapReady(StrategyResult& strategyResult);

		/** @brief get ? */
		//virtual MapInfoEntryPtr
		//getMasterBurst(MapInfoEntryPtr burst) const;
		//virtual MapInfoEntryPtr
		//getMasterBurst() const;

		//double symbolDuration; // Python parameter
		//bool txMode;           // Python parameter

		struct Colleagues {
		  Colleagues() {queue=NULL;grouper=NULL;registry=NULL;dsastrategy=NULL;dsafbstrategy=NULL;apcstrategy=NULL;};
			queue::QueueInterface* queue;
			grouper::GroupingProviderInterface* grouper;
			RegistryProxyInterface* registry;
			dsastrategy::DSAStrategyInterface* dsastrategy;
			dsastrategy::DSAStrategyInterface* dsafbstrategy;
			apcstrategy::APCStrategyInterface* apcstrategy;
		} colleagues;

		struct Friends {
			Friends() {ofdmaProvider=NULL;};
			wns::service::phy::ofdma::BFInterface* ofdmaProvider;
		} friends;

		//wns::scheduler::PowerCapabilities txPowerCapabilities;

		/** @brief PyConfig Attribute:
		    flag to determine whether the strategy is a PowerControl Slave */
		//bool powerControlSlave; // parameter from Python

		/** @brief there are three positions for the scheduler... */
		//PowerControlType powerControlType;

		/** @brief from friends.ofdmaProvider */
		//bool eirpLimited;

		/** @brief Python Config View */
		wns::pyconfig::View pyConfig;
		/** @brief Logger */
		wns::logger::Logger logger;

	protected:
		/** @brief default implementation to support the old scheduler strategies.
		    Please overload in all new strategies. */
		virtual StrategyResult
		doStartScheduling(SchedulerStatePtr schedulerState,
				  SchedulingMapPtr schedulingMap);

		/** @brief old interface to support the old scheduler strategies.
		    Please do not use anymore. */
		virtual void
		doStartScheduling(int fChannels, int maxBeams, simTimeType slotLength);

		/** @brief helper method to support the old scheduler strategies.
		    Replaces
		    bursts.push_back(currentBurst);
		    by
		    bursts_push_back(currentBurst);
		*/
		virtual void
		bursts_push_back(MapInfoEntryPtr bursts);
		/** @brief helper method to support the old scheduler strategies. */
		virtual void
		bursts_push_back_compound(wns::ldk::CompoundPtr pdu);
		/** @brief helper method to support the old scheduler strategies. */
		virtual bool
		isEirpLimited();

		/** @brief call this method after a compound has been scheduled
		    by any of the strategies.
		    Calls the callback().
		    Old method. Obsolete for new strategies. */
		void compoundReady(unsigned int fSlot,
				   simTimeType startTime, simTimeType endTime, UserID user,
				   const wns::ldk::CompoundPtr& pdu, unsigned int beam,
				   wns::service::phy::ofdma::PatternPtr pattern,
				   MapInfoEntryPtr burst,
				   const wns::service::phy::phymode::PhyModeInterface& phyMode,
				   wns::Power requestedTxPower,
				   wns::CandI estimatedCandI);

		/** @brief helper method to support the old scheduler strategies. */
		/** @brief return info about txPower (in slave scheduling mode) */
		wns::Power getTxPower() const;

	private:
		/** @brief set empty PhyMode which means full freedom to decide (master scheduling) */
		//virtual void clearPhyModePtr();
		/** @brief set default PhyMode when not deciding adaptively (slave scheduling) */
		//virtual void setPhyModePtr(wns::service::phy::phymode::PhyModeInterfacePtr _phyModePtr);
		/** @brief fresh resource unit on which a master scheduler can schedule */
		//virtual void clearMasterBurst();
		/** @brief define the recource unit on which a slave scheduler (UL) can schedule */
		//virtual void setMasterBurst(MapInfoEntryPtr burst);
		/** @brief tell me whose callBack is to be called when scheduling */
		//virtual void setCallBack(CallBackInterface* parent);
		/** @brief tell me whose callBack is to be called when scheduling */
		//virtual CallBackInterface* getCallBack();

		/** @brief constant PhyMode if not set adaptively */
		//wns::service::phy::phymode::PhyModeInterfacePtr defaultPhyModePtr;
		/** @brief method called in derived classes for every scheduled MapInfoEntry (subChannel) */
		//CallBackInterface* callBack;
		/** @brief recource unit on which a slave scheduler (UL) can schedule */
		//MapInfoEntryPtr masterBurst;
		/** @brief scheduler state variables/parameters */
		// this is the only state variable. But it is only a shadow of the state used in doStartScheduling()
		SchedulerStatePtr schedulerState;
	}; // class Strategy


	/**
	 *  @brief very lightweight interface to distinguish between different purpose
	 *  strategies. Besides being without functionality now, it would better be
	 *  called RxStrategy, because the classes derived from this do exactly
	 *  that: master-scheduling the UL (i.e. Rx-Phase)
	 *  @todo pab (2007-07-30): remove this interface entirely
	 */
      /*
	class ULStrategy :
		public Strategy,
		public ULAspect
	{
	public:
		virtual ~ULStrategy(){}
		ULStrategy(const wns::pyconfig::View& config);
	};
      */
}}} // namespace wns::scheduler::strategy
#endif // WNS_SCHEDULER_STRATEGY_STRATEGY_HPP


