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

#include <WNS/scheduler/harq/HARQ.hpp>
#include <boost/bind.hpp>

using namespace wns::scheduler::harq;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    HARQ,
    HARQInterface,
    "harq",
    wns::PyConfigViewCreator);

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    UniformRandomDecoder,
    IDecoder,
    "UniformRandomDecoder",
    wns::PyConfigViewCreator);

UniformRandomDecoder::UniformRandomDecoder(const wns::pyconfig::View& config):
    dis_(new wns::distribution::StandardUniform()),
    initialPER_(config.get<double>("initialPER")),
    rolloffFactor_(config.get<double>("rolloffFactor"))
{
}

bool
UniformRandomDecoder::canDecode(const wns::ldk::harq::softcombining::Container<wns::scheduler::SchedulingTimeSlotPtr>& c)
{
    int numTransmissions = 0;

    for (int ii=0; ii < c.getNumRVs(); ++ii)
    {
        numTransmissions += c.getEntriesForRV(ii).size();
    }

    double threshold = pow(initialPER_, numTransmissions * rolloffFactor_);

    //MESSAGE_BEGIN(NORMAL, logger_, m, "");
    //m << "Effective PER is " << threshold;
    //MESSAGE_END();

    if ((*dis_)() > threshold)
    {
        return true;
    }
    return false;
}

HARQ::HARQ(const wns::pyconfig::View& config):
    numSenderProcesses_(config.get<int>("numSenderProcesses")),
    numReceiverProcesses_(config.get<int>("numReceiverProcesses")),
    numRVs_(config.get<int>("numRVs")),
    logger_(config.get("logger")),
    config_(config)
{
}

HARQ::~HARQ()
{
}

void
HARQ::storeSchedulingTimeSlot(const wns::scheduler::SchedulingTimeSlotPtr& timeSlot)
{
    MESSAGE_SINGLE(VERBOSE, logger_, "storeSchedulingMap");

    if (!timeSlot->harq.enabled)
    {
        MESSAGE_SINGLE(VERBOSE, logger_, "storeSchedulingMap: HARQ is disabled. Skipping.");
        return;
    }

    if (timeSlot->isEmpty())
    {
        MESSAGE_SINGLE(NORMAL, logger_, "storeSchedulingMap: time slot is empty");
        return;
    }

    if (!timeSlot->harq.NDI)
    {
        MESSAGE_SINGLE(NORMAL, logger_, "storeSchedulingMap: Found a retransmission. Skipping.");
        return;
    }

    /**
     * @todo dbn: Check if timeSlot->getUserID is right in uplink
     */
    if (!harqEntities_.knows(timeSlot->getUserID()))
    {
        MESSAGE_SINGLE(NORMAL, logger_, "Creating new HARQEntity for UserID " << timeSlot->getUserID()->getName() );
        harqEntities_.insert(timeSlot->getUserID(), new HARQEntity(config_.get("harqEntity"), numSenderProcesses_, numReceiverProcesses_, numRVs_, logger_));
    }

    HARQEntity* ent = harqEntities_.find(timeSlot->getUserID());
    assure(ent->hasCapacity(), "The HARQ Entity for user " << timeSlot->getUserID()->getName() << " does not have anymore capacity!");

    // now store the SchedulingTimeSlotPtr
    /**
     * @todo dbn: Need to ask for permission. Use ent->hasCapacity
     */
    ent->newTransmission(timeSlot);
}

bool
HARQ::canDecode(const wns::scheduler::SchedulingTimeSlotPtr& timeslot)
{
    MESSAGE_SINGLE(VERBOSE, logger_, "onTimeSlotReceived");
    if (!timeslot->harq.enabled)
    {
        MESSAGE_SINGLE(VERBOSE, logger_, "onTimeSlotReceived: HARQ is disabled. Skipping.");
        return true;
    }

    if (!harqEntities_.knows(timeslot->getUserID()))
    {
        MESSAGE_SINGLE(NORMAL, logger_, "Creating new HARQEntity for UserID " << timeslot->getUserID()->getName() );
        harqEntities_.insert(timeslot->getUserID(), new HARQEntity(config_.get("harqEntity"), numSenderProcesses_, numReceiverProcesses_, numRVs_, logger_));
    }
    HARQEntity* ent = harqEntities_.find(timeslot->getUserID());
    /**
     * @todo dbn: Need to ask for permission. Use ent->hasCapacity
     */
    return ent->canDecode(timeslot);
}

wns::scheduler::SchedulingTimeSlotPtr
HARQ::nextRetransmission()
{
    for (HARQEntityContainer::const_iterator it = harqEntities_.begin();
         it != harqEntities_.end();
         ++it)
    {
        wns::scheduler::SchedulingTimeSlotPtr r;
        r = it->second->nextRetransmission();
        if (r != NULL)
        {
            return r;
        }
    }
    return wns::scheduler::SchedulingTimeSlotPtr();
}

HARQEntity::HARQEntity(const wns::pyconfig::View& config, int numSenderProcesses, int numReceiverProcesses, int numRVs, wns::logger::Logger logger):
    numSenderProcesses_(numSenderProcesses),
    numReceiverProcesses_(numReceiverProcesses),
    numRVs_(numRVs),
    logger_(logger)
{
    for (int ii=0; ii < numSenderProcesses_; ++ii)
    {
        senderProcesses_.push_back(HARQSenderProcess(this, ii, numRVs_, logger_));
    }

    for (int ii=0; ii < numReceiverProcesses_; ++ii)
    {
        receiverProcesses_.push_back(HARQReceiverProcess(config.get("receiverConfig"), this, ii, numRVs_, logger_));
    }
}

void
HARQEntity::newTransmission(const wns::scheduler::SchedulingTimeSlotPtr& timeslot)
{
    if (!timeslot->harq.enabled)
    {
        return;
    }

    assure(hasCapacity(), "HARQ Entity is busy but you wanted to start a new transmission!");

    // find free sender process
    for (int ii=0; ii < numSenderProcesses_; ++ii)
    {
        if (senderProcesses_[ii].hasCapacity())
        {
            senderProcesses_[ii].newTransmission(timeslot);
            break;
        }
    }
}

bool
HARQEntity::hasCapacity()
{
    for (int ii=0; ii < numSenderProcesses_; ++ii)
    {
        // Any of my send processes idle?
        if (senderProcesses_[ii].hasCapacity())
        {
            return true;
        }
    }
    return false;
}

bool
HARQEntity::canDecode(const wns::scheduler::SchedulingTimeSlotPtr& timeslot)
{
    if (!timeslot->harq.enabled)
    {
        return true;
    }

    assure(timeslot->harq.processID < receiverProcesses_.size(), "Invalid receiver process " << timeslot->harq.processID);
    return receiverProcesses_[timeslot->harq.processID].canDecode(timeslot);
}

void
HARQEntity::enqueueRetransmission(wns::scheduler::SchedulingTimeSlotPtr& timeslot)
{
    pendingRetransmissions_.push_back(timeslot);
}

wns::scheduler::SchedulingTimeSlotPtr
HARQEntity::nextRetransmission()
{
    if (pendingRetransmissions_.size() > 0)
    {
        wns::scheduler::SchedulingTimeSlotPtr r = pendingRetransmissions_.front();
        pendingRetransmissions_.pop_front();
        return r;
    }
    return wns::scheduler::SchedulingTimeSlotPtr();
}

HARQReceiverProcess::HARQReceiverProcess(const wns::pyconfig::View& config, HARQEntity* entity, int processID, int numRVs, wns::logger::Logger logger):
    entity_(entity),
    processID_(processID),
    numRVs_(numRVs),
    logger_(logger),
    receptionBuffer(numRVs)
{
    decoder_ = STATIC_FACTORY_NEW_INSTANCE(IDecoder, wns::PyConfigViewCreator, config.get("decoder"), config.get("decoder"));
}

bool
HARQReceiverProcess::canDecode(const wns::scheduler::SchedulingTimeSlotPtr& timeslot)
{
    MESSAGE_BEGIN(NORMAL, logger_, m, "HarqReceiverProcess::receive processID_ = " << processID_);
    m << ",RV = " << timeslot->harq.rv << ", RetryCounter=" << timeslot->harq.retryCounter;
    MESSAGE_END();

    if (timeslot->harq.NDI)
    {
        receptionBuffer.clear();
    }

    receptionBuffer.appendEntryForRV(timeslot->harq.rv, timeslot);

    if (decoder_->canDecode(receptionBuffer))
    {
        timeslot->harq.ackCallback();
        return true;
    }
    else
    {
        timeslot->harq.nackCallback();
        return false;
    }
}

HARQSenderProcess::HARQSenderProcess(HARQEntity* entity, int processID, int numRVs, wns::logger::Logger logger):
    entity_(entity),
    processID_(processID),
    numRVs_(numRVs),
    logger_(logger),
    timeslot_()
{
}

void
HARQSenderProcess::newTransmission(const wns::scheduler::SchedulingTimeSlotPtr& timeslot)
{
    assure(timeslot_ == NULL, "HARQ Process " << processID_ << " is busy but you wanted to start a new transmission!");
    timeslot_ = timeslot;
    timeslot->harq.processID = processID_;
    timeslot->harq.rv = 0;
    timeslot->harq.ackCallback = boost::bind(&HARQSenderProcess::ACK, this);
    timeslot->harq.nackCallback = boost::bind(&HARQSenderProcess::NACK, this);
}

bool
HARQSenderProcess::hasCapacity()
{
    return (timeslot_ == NULL);
}

void
HARQSenderProcess::ACK()
{
    MESSAGE_SINGLE(NORMAL, logger_, "HARQ process " << processID_ << " received ACK");
    timeslot_ = wns::scheduler::SchedulingTimeSlotPtr();
}

void
HARQSenderProcess::NACK()
{
    MESSAGE_SINGLE(NORMAL, logger_, "HARQ process " << processID_ << " received NACK");
    timeslot_->harq.NDI = false;
    timeslot_->harq.retryCounter++;
    entity_->enqueueRetransmission(timeslot_);
}
