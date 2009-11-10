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

#ifndef WNS_SCHEDULER_HARQ_HARQINTERFACE_HPP
#define WNS_SCHEDULER_HARQ_HARQINTERFACE_HPP

#include <WNS/scheduler/SchedulingMap.hpp>
#include <WNS/service/phy/power/PowerMeasurement.hpp>
#include <WNS/StaticFactory.hpp>
#include <WNS/PyConfigViewCreator.hpp>

namespace wns { namespace scheduler { namespace harq {

class HARQInterface
{
public:

    virtual ~HARQInterface() {};

    /**
     * @brief Called by the scheduler after the scheduling step has finished
     */
    virtual void
    storeSchedulingTimeSlot(const wns::scheduler::SchedulingTimeSlotPtr&) = 0;

    /**
     * @brief Called by the scheduler when a SchedulingTimeSlot is received from the peer
     */
    virtual bool
    canDecode(const wns::scheduler::SchedulingTimeSlotPtr&, const wns::service::phy::power::PowerMeasurementPtr&) = 0;

    virtual wns::scheduler::SchedulingTimeSlotPtr
    nextRetransmission() = 0;

    STATIC_FACTORY_DEFINE(HARQInterface, wns::PyConfigViewCreator);
};

} // harq
} // scheduler
} // wns

#endif // WNS_SCHEDULER_HARQ_HARQINTERFACE_HPP
