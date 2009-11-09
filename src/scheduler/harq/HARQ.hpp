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

#ifndef WNS_SCHEDULER_HARQ_HARQ_HPP
#define WNS_SCHEDULER_HARQ_HARQ_HPP

#include <WNS/scheduler/harq/HARQInterface.hpp>
#include <WNS/ldk/harq/softcombining/Container.hpp>
#include <WNS/distribution/Uniform.hpp>

#include <WNS/logger/Logger.hpp>
#include <WNS/container/Registry.hpp>

namespace wns { namespace scheduler { namespace harq {

class HARQEntity;

class IDecoder
{
public:

    virtual bool
    canDecode(const wns::ldk::harq::softcombining::Container<wns::scheduler::SchedulingTimeSlotPtr>&) = 0;
};
STATIC_FACTORY_DEFINE(IDecoder, wns::PyConfigViewCreator);

class UniformRandomDecoder:
    public IDecoder
{
public:
    UniformRandomDecoder(const wns::pyconfig::View&);

    virtual bool
    canDecode(const wns::ldk::harq::softcombining::Container<wns::scheduler::SchedulingTimeSlotPtr>&);
private:

    std::auto_ptr<wns::distribution::Distribution> dis_;

    double initialPER_;

    double rolloffFactor_;
};

class HARQReceiverProcess
{
public:
    HARQReceiverProcess(const wns::pyconfig::View&, HARQEntity*, int processID, int numRVs, const wns::logger::Logger);

    bool
    canDecode(const wns::scheduler::SchedulingTimeSlotPtr&);

private:
    HARQEntity* entity_;

    int processID_;

    int numRVs_;

    wns::logger::Logger logger_;

    wns::ldk::harq::softcombining::Container<wns::scheduler::SchedulingTimeSlotPtr> receptionBuffer;

    IDecoder* decoder_;
};

class HARQSenderProcess
{
public:
    HARQSenderProcess(HARQEntity*, int processID, int numRVs, const wns::logger::Logger);

    bool
    hasCapacity();

    void
    newTransmission(const wns::scheduler::SchedulingTimeSlotPtr& timeslot);

    void
    ACK();

    void
    NACK();

private:
    HARQEntity* entity_;

    int processID_;

    int numRVs_;

    wns::logger::Logger logger_;

    wns::scheduler::SchedulingTimeSlotPtr timeslot_;
};

class HARQEntity
{
public:
    HARQEntity(const wns::pyconfig::View&, int numSenderProcesses, int numReceiverProcesses, int numRVs, wns::logger::Logger logger);

    void
    newTransmission(const wns::scheduler::SchedulingTimeSlotPtr&);

    bool
    hasCapacity();

    bool
    canDecode(const wns::scheduler::SchedulingTimeSlotPtr&);

    void
    enqueueRetransmission(wns::scheduler::SchedulingTimeSlotPtr&);

    wns::scheduler::SchedulingTimeSlotPtr
    nextRetransmission();

private:
    std::vector<HARQSenderProcess> senderProcesses_;

    std::vector<HARQReceiverProcess> receiverProcesses_;

    std::list<wns::scheduler::SchedulingTimeSlotPtr> pendingRetransmissions_;

    int numSenderProcesses_;

    int numReceiverProcesses_;

    int numRVs_;

    wns::logger::Logger logger_;
};

class HARQ:
    public wns::scheduler::harq::HARQInterface
{
public:

    HARQ(const wns::pyconfig::View&);

    virtual ~HARQ();

    /**
     * @brief Called by the scheduler after the scheduling step has finished
     */
    virtual void
    storeSchedulingTimeSlot(const wns::scheduler::SchedulingTimeSlotPtr&);

    /**
     * @brief Called by the scheduler when a scheduling map is received from the peer
     *
     * The map is attached to all compounds that are transmitted. The HARQ must
     * accomplish duplicate detection and may only process one of the maps
     */
    virtual bool
    canDecode(const wns::scheduler::SchedulingTimeSlotPtr&);

    virtual wns::scheduler::SchedulingTimeSlotPtr
    nextRetransmission();

private:
    wns::logger::Logger logger_;

    typedef wns::container::Registry<wns::scheduler::UserID, HARQEntity*> HARQEntityContainer;

    HARQEntityContainer harqEntities_;

    int numSenderProcesses_;

    int numReceiverProcesses_;

    int numRVs_;

    wns::pyconfig::View config_;
};

} // harq
} // scheduler
} // wns

#endif // WNS_SCHEDULER_HARQ_HARQ_HPP
