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

#include <WNS/scheduler/strategy/dsastrategy/PersistentVoIP.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>

using namespace wns::scheduler;
using namespace wns::scheduler::strategy;
using namespace wns::scheduler::strategy::dsastrategy;

STATIC_FACTORY_REGISTER_WITH_CREATOR(PersistentVoIP,
                                     DSAStrategyInterface,
                                     "PersistentVoIP",
                                     wns::PyConfigViewCreator);

PersistentVoIP::PersistentVoIP(const wns::pyconfig::View& config)
    : DSAStrategy(config)
{
}

PersistentVoIP::~PersistentVoIP()
{
}

// call this before each timeSlot/frame
void
PersistentVoIP::initialize(SchedulerStatePtr schedulerState,
                         SchedulingMapPtr schedulingMap)
{
    DSAStrategy::initialize(schedulerState, schedulingMap); // must always initialize base class too

    int maxSubChannel = schedulerState->currentState->strategyInput->getFChannels();
    int numberOfTimeSlots = schedulerState->currentState->strategyInput->getNumberOfTimeSlots();
    int maxSpatialLayers = schedulerState->currentState->strategyInput->getMaxSpatialLayers();   

    tmp_ = 0; 
}

DSAResult
PersistentVoIP::getSubChannelWithDSA(RequestForResource& request,
                                   SchedulerStatePtr schedulerState,
                                   SchedulingMapPtr schedulingMap)
{
    //assure(request.user.isBroadcast() || resStart_.find(request.user.getNodeID()) != resStart_.end(),
    //    "No resources for user " + request.user.getName());

    // Give the broadcast channel the first resource
    if(request.user.isBroadcast())
    { 
        assure(channelIsUsable(0, 
                                0,
                                0,
                                request, 
                                schedulerState, 
                                schedulingMap), "First resource not available for broadcast");
        MESSAGE_SINGLE(NORMAL, logger, "getSubChannelWithDSA(): Granting resource: 0.0.0"
            << " to " << request.toString());
        DSAResult dsaResult;
        dsaResult.subChannel = 0;
        dsaResult.timeSlot = 0;
        dsaResult.spatialLayer = 0;
        return dsaResult;
    }
    else
    {
	std::cout << "DEBUG: " << request.cid << "\n";

	if(lastTime_.find(request.cid) == lastTime_.end())
        {
            lastTime_[request.cid] = wns::simulator::getEventScheduler()->getTime();
	    std::cout << "DEBUG First Time\n";
        }
        else
        {
            std::cout << "DEBUG Last: " << wns::simulator::getEventScheduler()->getTime() - lastTime_[request.cid] << " " << request.cid << "\n";
            lastTime_[request.cid] = wns::simulator::getEventScheduler()->getTime();
        }

        DSAResult dsaResult;

        if(tmp_ < 99)
	{
            tmp_++;
            dsaResult.subChannel = tmp_;
            dsaResult.timeSlot = 0;
            dsaResult.spatialLayer = 0;
            MESSAGE_SINGLE(NORMAL, logger, "getSubChannelWithDSA(): Granting resource: " << tmp_ << ".0.0");
        }
        else
        {
            MESSAGE_SINGLE(NORMAL, logger, "getSubChannelWithDSA(): no free subchannel");
        }

        return dsaResult;
    }
} // getSubChannelWithDSA
