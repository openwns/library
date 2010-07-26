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

#ifndef WNS_SCHEDULER_MOHARQ_HARQ_HPP
#define WNS_SCHEDULER_NOHARQ_HARQ_HPP

#include <WNS/scheduler/harq/HARQInterface.hpp>

namespace wns { namespace scheduler { namespace harq {

class NoHARQ:
    public wns::scheduler::harq::HARQInterface
{
public:
    NoHARQ(const wns::pyconfig::View&);

    virtual ~NoHARQ();

    virtual void
    storeSchedulingTimeSlot(const wns::scheduler::SchedulingTimeSlotPtr&);

    virtual void
    onTimeSlotReceived(const wns::scheduler::SchedulingTimeSlotPtr&, HARQInterface::TimeSlotInfo);

    virtual HARQInterface::DecodeStatusContainer
    decode();

    virtual int
    getNumberOfRetransmissions();

    virtual wns::scheduler::SchedulingTimeSlotPtr
    getNextRetransmission();

    virtual wns::scheduler::SchedulingTimeSlotPtr
    peekNextRetransmission() const;

    virtual void
    setDownlinkHARQ(HARQInterface* downlinkHARQ);

    virtual wns::scheduler::UserSet
    getPeersWithPendingRetransmissions() const;

    virtual int
    getNumberOfPeerRetransmissions(wns::scheduler::UserID peer) const;

    virtual void
    increaseScheduledPeerRetransmissionCounter(wns::scheduler::UserID peer);

    virtual void
    resetScheduledPeerRetransmissionCounter(wns::scheduler::UserID peer);

    virtual void
    resetAllScheduledPeerRetransmissionCounters();

private:
    HARQInterface::DecodeStatusContainer receivedNonHARQTimeslots_;
};

} // harq
} // scheduler
} // wns

#endif // WNS_SCHEDULER_NOHARQ_HARQ_HPP
