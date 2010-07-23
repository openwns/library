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
    class TimeSlotInfo
    {
    public:
        TimeSlotInfo(wns::service::phy::power::PowerMeasurementPtr pwrm, double distance, int sc):
            powerMeasurement_(pwrm),
            distance_(distance_),
            sc_(sc)
        {}

        wns::service::phy::power::PowerMeasurementPtr powerMeasurement_;
        double distance_;
        int sc_;
    };

    typedef std::pair<wns::scheduler::SchedulingTimeSlotPtr, TimeSlotInfo> DecodeStatusContainerEntry;
    typedef std::list<DecodeStatusContainerEntry> DecodeStatusContainer;

    virtual ~HARQInterface() {};

    /**
     * @brief Called by the scheduler after the scheduling step has finished
     */
    virtual void
    storeSchedulingTimeSlot(const wns::scheduler::SchedulingTimeSlotPtr&) = 0;

    /**
     * @brief Called by the scheduler when a SchedulingTimeSlot is received from the peer
     */
    virtual void
    onTimeSlotReceived(const wns::scheduler::SchedulingTimeSlotPtr&, TimeSlotInfo info) = 0;

    virtual DecodeStatusContainer
    decode() = 0;

    virtual int
    getNumberOfRetransmissions() = 0;

    /**
     * @brief Returns the next HARQ Retransmission block and removes it from
     * the pending retransmission list.
     *
     * No matter which size [bits]. No matter which PhyMode.
     * The order is defined internally (FCFS).
     */
    virtual wns::scheduler::SchedulingTimeSlotPtr
    getNextRetransmission() = 0;

    /**
     * @brief Returns the next HARQ Retransmission block, but keeps it in
     * the pending retransmission list.
     *
     * No matter which size [bits]. No matter which PhyMode.
     * The order is defined internally (FCFS).
     */
    virtual wns::scheduler::SchedulingTimeSlotPtr
    peekNextRetransmission() const = 0;

    /**
     * @brief Set a downlink HARQ. Probably only applicable in uplink master schedulers
     */
    virtual void
    setDownlinkHARQ(HARQInterface* downlinkHARQ) = 0;

    /**
     * @brief Returns the peers that have pending retransmissions
     * Intended for uplink master scheduling.
     */
    virtual wns::scheduler::UserSet
    getPeersWithPendingRetransmissions() const = 0;

    /**
     * @brief Return the number of retransmissions that are pending for a peer
     * Intended for uplink master scheduling
     */
    virtual int
    getNumberOfPeerRetransmissions(wns::scheduler::UserID peer) const = 0;

    /**
     * @brief Increases the scheduled peer retransmission counter for user peer. Used for TDD if multiple frames are scheduled in advance
     */
    virtual void
    increaseScheduledPeerRetransmissionCounter(wns::scheduler::UserID peer) = 0;

    /**
     * @brief Resets the scheduled peer retransmission counter for user peer. Used for TDD if multiple frames are scheduled in advance
     */
    virtual void
    resetScheduledPeerRetransmissionCounter(wns::scheduler::UserID peer) = 0;

    /**
     * @brief Resets all the scheduled peer retransmission counters for all peers. Used for TDD if multiple frames are scheduled in advance
     */
    virtual void
    resetAllScheduledPeerRetransmissionCounters() = 0;

    STATIC_FACTORY_DEFINE(HARQInterface, wns::PyConfigViewCreator);
};

} // harq
} // scheduler
} // wns

#endif // WNS_SCHEDULER_HARQ_HARQINTERFACE_HPP
