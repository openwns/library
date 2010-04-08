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

#include <WNS/scheduler/strategy/dsastrategy/Fixed.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>

using namespace wns::scheduler;
using namespace wns::scheduler::strategy;
using namespace wns::scheduler::strategy::dsastrategy;

STATIC_FACTORY_REGISTER_WITH_CREATOR(Fixed,
                                     DSAStrategyInterface,
                                     "Fixed",
                                     wns::PyConfigViewCreator);

Fixed::Fixed(const wns::pyconfig::View& config)
    : DSAStrategy(config)
{
}

Fixed::~Fixed()
{
}

// call this before each timeSlot/frame
void
Fixed::initialize(SchedulerStatePtr schedulerState,
                         SchedulingMapPtr schedulingMap)
{
    DSAStrategy::initialize(schedulerState,schedulingMap); // must always initialize base class too

    int maxSubChannel = schedulerState->currentState->strategyInput->getFChannels();
    int numberOfTimeSlots = schedulerState->currentState->strategyInput->getNumberOfTimeSlots();
    int maxSpatialLayers = schedulerState->currentState->strategyInput->getMaxSpatialLayers();

    int prio = schedulerState->currentState->getCurrentPriority();
    wns::scheduler::ConnectionSet conns = colleagues.registry->getConnectionsForPriority(prio);

    std::set<wns::scheduler::UserID> userIDs;
    wns::scheduler::ConnectionSet::iterator it;

    for(it = conns.begin(); it != conns.end(); it++)
        userIDs.insert(colleagues.registry->getUserForCID(*it));

    unsigned int numberOfFlows = userIDs.size();
    unsigned int numberOfResources = maxSubChannel * numberOfTimeSlots * maxSpatialLayers;

    MESSAGE_SINGLE(NORMAL, logger, "getSubChannelWithDSA: Distributing " 
        << numberOfFlows << "Users on " << numberOfResources << " resources.");
}

DSAResult
Fixed::getSubChannelWithDSA(RequestForResource& request,
                                   SchedulerStatePtr schedulerState,
                                   SchedulingMapPtr schedulingMap)
{
    DSAResult dsaResult;
                    
    return dsaResult;
} // getSubChannelWithDSA
