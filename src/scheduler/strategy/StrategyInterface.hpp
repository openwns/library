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

#ifndef WNS_SCHEDULER_STRATEGY_STRATEGYINTERFACE_HPP
#define WNS_SCHEDULER_STRATEGY_STRATEGYINTERFACE_HPP

#include <WNS/service/phy/phymode/PhyModeInterface.hpp>
#include <WNS/service/phy/ofdma/DataTransmission.hpp>
#include <WNS/scheduler/strategy/apcstrategy/APCStrategyInterface.hpp>
#include <WNS/scheduler/strategy/dsastrategy/DSAStrategyInterface.hpp>
//#include <WNS/scheduler/MapInfoProviderInterface.hpp>
#include <WNS/scheduler/CallBackInterface.hpp>
#include <WNS/scheduler/SchedulingMap.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>
#include <WNS/StaticFactory.hpp>
#include <WNS/PyConfigViewCreator.hpp>
#include <WNS/SmartPtr.hpp>

namespace wns { namespace scheduler {
        namespace queue {
            class QueueInterface;
        }
        namespace grouper {
            class GroupingProviderInterface;
        }
        namespace strategy { namespace tests {
                class StrategyTest; // friend
            }}

        class RegistryProxyInterface;
        class CallBackInterface;
    }} // forward declarations

namespace wns { namespace scheduler { namespace strategy {
            /** @brief container for the basic datastructures
                that the scheduler strategy needs to work on.
                You can even derive from it and add more elements if you need them. */
            class StrategyInput // NEW [rs]
            {
            public:
                /** @brief constructor for master scheduling */
                StrategyInput(int _fChannels,
                              double _slotLength,
                              int _maxBeams,
                              CallBackInterface* _callBackObject);
                /** @brief constructor for slave scheduling */
                StrategyInput(MapInfoEntryPtr _mapInfoEntryFromMaster,
                              CallBackInterface* _callBackObject);
                /** @brief generic constructor for both types of scheduling */
                StrategyInput(int _fChannels,
                              double _slotLength,
                              int _maxBeams,
                              MapInfoEntryPtr _mapInfoEntryFromMaster,
                              CallBackInterface* _callBackObject);
                StrategyInput();
                ~StrategyInput();
                /** @brief set (optional!) parameter frameNr */
                virtual void setFrameNr(int _frameNr);
                virtual int getFrameNr() const { return frameNr; };
                virtual int getFChannels() const { return fChannels; };
                virtual double getSlotLength() const { return slotLength; };
                virtual int getMaxBeams() const { return maxBeams; };
                virtual bool frameNrIsValid() const;
                /** @brief set (optional!) parameter defaultPhyMode
                    If this is set, no other PhyMode will be used.
                    No AMC for the simple (old) strategies */
                virtual void setDefaultPhyMode(wns::service::phy::phymode::PhyModeInterfacePtr _phyModePtr);
                /** @brief set (optional!) parameter defaultTxPower
                    If this is set, no other TxPower will be used.
                    No APC for the simple (old) strategies */
                virtual void setDefaultTxPower(wns::Power _txPower);
                /** @brief get empty SchedulingMap configured with the correct resource dimensions. */
                virtual wns::scheduler::SchedulingMapPtr getEmptySchedulingMap() const;
                /** @brief get parameter inputSchedulingMap. */
                virtual wns::scheduler::SchedulingMapPtr getInputSchedulingMap() const;
                /** @brief set (optional!) parameter inputSchedulingMap
                    If this is set, the scheduler operates on previously allocated resources */
                virtual void setInputSchedulingMap(wns::scheduler::SchedulingMapPtr _inputSchedulingMap);
                virtual std::string toString() const;
            public:
                /** @brief size of resources in frequency-direction */
                int fChannels;
                /** @brief size of resources in time-direction */
                double slotLength;
                /** @brief true: use beamforming if(maxBeams>1).
                    false: use MIMO if(maxBeams>1) */
                bool beamforming;
                /** @brief size of resources in spatial direction.
                    This can be beamforming beams (available for WiMAC)
                    or MIMO paths (not yet available). */
                int maxBeams;
                /** @brief pointer to caller which has callBack() implemented.
                    A value of NULL means: don't use. */
                CallBackInterface* callBackObject;
                /** @brief A slave scheduler gets this masterBurst from the masterScheduler
                    via maps (DL signaling).
                    Is a SmartPtr. */
                MapInfoEntryPtr mapInfoEntryFromMaster;
                /** @brief Any scheduler can get a preassigned schedulingMap.
                    E.g. for master-slave scheduling
                    or for using subChannelIsUsable pre-allocations
                    or to do multiple scheduling rounds over the same frame
                    or to encorporate static allocations
                    Is a SmartPtr. */
                wns::scheduler::SchedulingMapPtr inputSchedulingMap;
                /** @brief constant PhyMode if not set adaptively */
                wns::service::phy::phymode::PhyModeInterfacePtr defaultPhyModePtr;
                /** @brief constant txPower if not set adaptively */
                wns::Power defaultTxPower;
                /** @brief frameNr is used for advance scheduling */
                int frameNr;
                // TODO: put members into 'protected' area and provide get* and set* functions.
            }; // class StrategyInput

            /** @brief container for the basic datastructures
                that contain the scheduler sdtrategy results */
            class StrategyResult // NEW [rs]
                    : virtual public wns::RefCountable
            {
            public:
                /** @brief constructor without SDMA-Grouping */
                StrategyResult(wns::scheduler::SchedulingMapPtr _schedulingMap,
                               wns::scheduler::MapInfoCollectionPtr _bursts);
                /** @brief constructor with SDMA-Grouping */
                StrategyResult(wns::scheduler::SchedulingMapPtr _schedulingMap,
                               wns::scheduler::MapInfoCollectionPtr _bursts,
                               wns::scheduler::GroupingPtr _sdmaGrouping);
                virtual ~StrategyResult();
                virtual std::string toString() const;
                /** @brief obtain percentage of resources allocated during last round. */
                virtual float getResourceUsage() const;
                /** @brief count number of compounds in bursts */
                virtual int getNumberOfCompoundsInBursts() const;
                /** @brief delete all compounds in bursts */
                virtual void deleteCompoundsInBursts();
            public:
                /** @brief full scheduling result (new for OFDMA) */
                wns::scheduler::SchedulingMapPtr schedulingMap;
                /** @brief full scheduling result (used in WinProSt) */
                MapInfoCollectionPtr bursts; // historic name
                /** @brief Grouping contains antenna patterns for beamforming.
                    This is a wns::service::phy::ofdma::PatternPtr per user/peer.
                    @see SchedulerTypes.hpp */
                //wns::scheduler::AntennaPatternsPerUser antennaPatterns;
                wns::scheduler::GroupingPtr sdmaGrouping;
            }; // class StrategyResult

            /** @brief This makes it easy to keep StrategyResults in a vector[frameNr] */
            typedef SmartPtr<StrategyResult> StrategyResultPtr;

            /** @brief one StrategyResult entry per timeFrame if keepResultHistory==true */
            typedef std::vector< StrategyResultPtr > StrategyResultHistory;

            /** @brief
             * provides methods used by ResourceSchedulers to access certain WNS
             * scheduler strategies
             */
            class StrategyInterface
            {
            public:
                virtual ~StrategyInterface() {};

                /** @brief
                 * returns member var bursts which is set in the derived
                 * strategies in doStartScheduling by bursts.push_back(currentBurst)
                 */
                virtual MapInfoCollectionPtr getMapInfo() const = 0;

                /** @brief called in specialized scheduler (WinProSt,WiMAC,...)
                    for defining the objects which operate closely together:
                    strategy,queues,spatialGrouper,registryProxy */
                virtual void setColleagues(queue::QueueInterface* _queue,
                                           grouper::GroupingProviderInterface* _grouper,
                                           RegistryProxyInterface* _registry) = 0;

                /** @brief can be overloaded by derived classes to perform initialization code.
                    The derived class should call this base class method first. */
                virtual void onColleaguesKnown() = 0;

                /** @brief called in specialized scheduler (WinProSt,WiMAC,...)
                    for defining friends */
                virtual void setFriends(wns::service::phy::ofdma::BFInterface* ) = 0;

                /** @brief (for AP) determines the order of PDUs in the given full set of resources.
                    calls doStartScheduling() of derived class.
                    @param StrategyInput contains parameters set by the caller.
                    @return StrategyResult collection. All set inside.
                */
                virtual StrategyResult
                startScheduling(const StrategyInput& StrategyInput) = 0;

                /** @brief OBSOLETE!
                    (for AP) determines the order of PDUs in the given full set of resources.
                    calls doStartScheduling() of derived class. */
                virtual void
                startScheduling(int fChannels,
                                int maxBeams,
                                double slotLength,
                                CallBackInterface* parent);
                /** @brief OBSOLETE!
                    (for UT,FRS) determines the order in the assigned set of
                    resources (slave scheduling)
                    calls doStartScheduling() of derived class */
                virtual void
                startScheduling(MapInfoEntryPtr burst,
                                CallBackInterface* parent);

                /** @brief there are three positions for the scheduler.
                    Only used in WinProSt. */
                virtual wns::scheduler::SchedulerSpotType
                getSchedulerSpotType() const = 0;

                /** @brief indicates type of ResourceScheduler (TX or RX).
                    Only used in WinProSt. */
                virtual bool isTx() const = 0;


                /** @brief indicates if strategy is of 'old' (before 2009) or 'new' type (>=2009). Used in assertions. */
                virtual bool isNewStrategy() const = 0;

                /** @brief retrieve info about a peer user's power capabilities (for master UL scheduling) */
                virtual wns::scheduler::PowerCapabilities
                getPowerCapabilities(const UserID user) const = 0;

                /** @brief obtain percentage of resources allocated during last round
                 */
                virtual float getResourceUsage() const = 0;

                /**
                 * @brief returns bursts.size(); used e.g. in WinProst::ResourceScheduler::getNumBursts()
                 * Belongs to the "old" interface. Should become obsolete.
                 */
                virtual int getNumBursts() const = 0;

                /** @brief delete previous Map content.
                    Old interface. Not required for new schedulers. */
                virtual void clearMap() = 0;

            protected:
                /** @brief (non-virtual-interface) called in startScheduling(..)
                    @param SchedulerStatePtr schedulerState
                */
                //virtual void doStartScheduling(int fChannels, int maxBeams, simTimeType slotLength) = 0;
                virtual StrategyResult
                doStartScheduling(SchedulerStatePtr schedulerState,
                                  SchedulingMapPtr schedulingMap) = 0;

                /** @brief old interface to support the old scheduler strategies.
                    Please do not use anymore. */
                virtual void
                doStartScheduling(int fChannels, int maxBeams, simTimeType slotLength) = 0;

                /** @brief The strategies need a new state.
                    Implement this in the derived classes. */
                virtual SchedulerStatePtr
                getNewSchedulerState() = 0;
                /** @brief The state is revolved before a new doStartScheduling() call.
                    Implement this in the derived classes. */
                virtual SchedulerStatePtr
                revolveSchedulerState(const StrategyInput& strategyInput) = 0;
                /** @brief Get the scheduler state here.
                    Implement this in the derived classes. */
                virtual SchedulerStatePtr
                getSchedulerState() = 0;

            private:
                /** @brief set empty PhyMode which means full freedom to decide (master scheduling) */
                //virtual void clearPhyModePtr() = 0;
                /** @brief for slave scheduling, set the pre-defined phyMode
                 * decided by the master scheduler */
                //virtual void setPhyModePtr(wns::service::phy::phymode::PhyModeInterfacePtr _phyModePtr) = 0;
                /** @brief for slave scheduling, set the master burst */
                //virtual void setMasterBurst(MapInfoEntryPtr burst) = 0;
                /** @brief tell me whose callBack is to be called when scheduling
                    decision is ready (e.g. ResourceScheduler in WinProSt)
                    this is set each time when startScheduling is called */
                //virtual void setCallBack(CallBackInterface* parent) = 0;
                friend class wns::scheduler::strategy::tests::StrategyTest;
            };

            typedef wns::PyConfigViewCreator<StrategyInterface, StrategyInterface> StrategyCreator;
            typedef wns::StaticFactory<StrategyCreator> StrategyFactory;

        }}} // namespace wns::scheduler::strategy
#endif // WNS_SCHEDULER_STRATEGY_STRATEGYINTERFACE_HPP


