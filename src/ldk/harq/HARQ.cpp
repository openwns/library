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

#include <WNS/ldk/harq/HARQ.hpp>
#include <WNS/ldk/harq/softcombining/UniformRandomDecoder.hpp>

using namespace wns::ldk::harq;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    HARQ,
    wns::ldk::FunctionalUnit,
    "wns.harq.HARQ",
    wns::ldk::FUNConfigCreator);

HARQ::HARQSenderProcess::HARQSenderProcess(int processID,
                                           int numRVs,
                                           HARQ* entity,
                                           wns::logger::Logger logger):
    processID_(processID),
    numRVs_(numRVs),
    entity_(entity),
    logger_(logger),
    buffer_(),
    waitingForFeedback_(false),
    retransmissionCounter_(0)
{
    assure(entity_!=NULL, "No HARQ entity available. This should not have happened");

    MESSAGE_BEGIN(NORMAL, logger_, m, "");
    m << "Process " << processID_ << " created";
    MESSAGE_END();
}

bool
HARQ::HARQSenderProcess::hasCapacity() const
{

    if (buffer_ == NULL)
    {
        return true;
    }

    return false;
}

void
HARQ::HARQSenderProcess::enqueueTransmission(const wns::ldk::CompoundPtr& compound)
{
    assure(entity_!=NULL, "No HARQ entity available. This should not have happened");
    HARQCommand* command = entity_->getCommand(compound->getCommandPool());
    command->peer.type = HARQCommand::I;
    command->peer.NDI = true;
    command->peer.processId = processID_;
    buffer_ = compound;

    entity_->addToSendQueue(buffer_->copy());
}

void
HARQ::HARQSenderProcess::ackReceived()
{
    buffer_ = wns::ldk::CompoundPtr();
    waitingForFeedback_ = false;
    retransmissionCounter_ = 0;

    MESSAGE_BEGIN(NORMAL, logger_, m, "");
    m << "Process " << processID_ << " received ACK";
    MESSAGE_END();
}

void
HARQ::HARQSenderProcess::nackReceived()
{
    waitingForFeedback_ = false;
    retransmissionCounter_++;

    wns::ldk::CompoundPtr retransmission = buffer_->copy();

    HARQCommand* command = entity_->getCommand(retransmission->getCommandPool());
    command->peer.NDI = false;
    command->peer.rv = (command->peer.rv + 1) % numRVs_;

    entity_->addToSendQueue(retransmission);

    MESSAGE_BEGIN(NORMAL, logger_, m, "");
    m << "Process " << processID_ << " received NACK";
    MESSAGE_END();
}

HARQ::HARQReceiverProcess::HARQReceiverProcess(int processID,
                                               int numRVs,
                                               HARQ* entity,
                                               wns::logger::Logger logger):
    processID_(processID),
    numRVs_(numRVs),
    entity_(entity),
    logger_(logger),
    receptionBuffer_(numRVs),
    decoder_(new softcombining::UniformRandomDecoder())
{
    assure(entity_ != NULL, "No HARQ entity available. This should not have happened");

    MESSAGE_BEGIN(NORMAL, logger_, m, "");
    m << "Process " << processID_ << " created";
    MESSAGE_END();
}

void
HARQ::HARQReceiverProcess::receive(const wns::ldk::CompoundPtr& compound)
{
    assure(entity_ != NULL, "No HARQ entity available. This should not have happened");

    HARQCommand* command = entity_->getCommand(compound->getCommandPool());

    receptionBuffer_.appendCompoundForRV(command->peer.rv, compound);

    CommandPool* ackPCI = entity_->getFUN()->getProxy()->createReply(compound->getCommandPool(), entity_);
    wns::ldk::CompoundPtr ack_ = wns::ldk::CompoundPtr(new Compound(ackPCI));
    HARQCommand* harqCommand = entity_->activateCommand(ackPCI);
    harqCommand->peer.processId = processID_;

    if (decoder_->canDecode(receptionBuffer_))
    {
        harqCommand->peer.type = HARQCommand::ACK;
        entity_->addToSendQueue(ack_);

        if(entity_->getDeliverer()->size())
            entity_->getDeliverer()->getAcceptor(compound)->onData(compound);
    }
    else
    {
        harqCommand->peer.type = HARQCommand::NACK;
        entity_->addToSendQueue(ack_);
    }
}

HARQ::HARQ(wns::ldk::fun::FUN* fuNet, const wns::pyconfig::View& config) :
    fu::Plain<HARQ, HARQCommand>(fuNet),
    numSenderProcesses_(config.get<int>("numSenderProcesses")),
    numReceiverProcesses_(config.get<int>("numReceiverProcesses")),
    numRVs_(config.get<int>("numRVs")),
    logger_(config.get("logger"))
{
    for (int ii=0; ii < numSenderProcesses_; ++ii)
    {
        senderProcesses_.push_back(HARQSenderProcess(ii, numRVs_, this, logger_));
    }

    for (int ii=0; ii < numReceiverProcesses_; ++ii)
    {
        receiverProcesses_.push_back(HARQ::HARQReceiverProcess(ii, numRVs_, this, logger_));
    }
}

HARQ::~HARQ()
{
}

void
HARQ::onFUNCreated()
{
}  // onFUNCreated

bool
HARQ::hasCapacity() const
{
    for (int ii=0; ii < numSenderProcesses_; ++ii)
    {
        // Any of my send processes idle?
        if (senderProcesses_[ii].hasCapacity())
        {
            return true;
        }
    }

    // Damn! All busy
    return false;
}

const wns::ldk::CompoundPtr
HARQ::hasSomethingToSend() const
{
    if (!sendQueue_.empty())
    {
        return sendQueue_.front();
    }

    return wns::ldk::CompoundPtr();
}

wns::ldk::CompoundPtr
HARQ::getSomethingToSend()
{
    if (!sendQueue_.empty())
    {
        wns::ldk::CompoundPtr it = sendQueue_.front();
        sendQueue_.pop_front();

        return it;
    }

    return wns::ldk::CompoundPtr();
}

void
HARQ::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    activateCommand(compound->getCommandPool());

    for (int ii=0; ii < numSenderProcesses_; ++ii)
    {
        // Any of my send processes idle?
        if (senderProcesses_[ii].hasCapacity())
        {
            MESSAGE_BEGIN(NORMAL, logger_, m, getFUN()->getName());
            m << " HARQ process " << ii << " is now active";
            MESSAGE_END();

            HARQCommand* command = getCommand(compound->getCommandPool());
            command->peer.NDI = true;
            command->peer.processId = ii;
            senderProcesses_[ii].enqueueTransmission(compound);
            break;
        }
    }
}

void
HARQ::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    HARQCommand* command = getCommand(compound->getCommandPool());

    int processId = command->peer.processId;
    bool ndi = command->peer.NDI;
    HARQCommand::FrameType commandType = command->peer.type;

    switch(commandType)
    {
    case HARQCommand::I:
        MESSAGE_BEGIN(NORMAL, logger_, m, getFUN()->getName());
        m << " received compound for process " << processId;
        if (ndi)
        {
            m << " (NDI)";
        }
        MESSAGE_END();

        assure(processId < numReceiverProcesses_ && processId >= 0, "No receiver process for this ProcessId");

        receiverProcesses_[processId].receive(compound);

        break;

    case HARQCommand::ACK:

        assure(processId < numSenderProcesses_ && processId >= 0, "No receiver process for this ProcessId");

        MESSAGE_BEGIN(NORMAL, logger_, m, getFUN()->getName());
        m << " received ACK compound for process " << processId;
        if (ndi)
        {
            m << " (NDI)";
        }
        MESSAGE_END();

        senderProcesses_[processId].ackReceived();

        break;

    case HARQCommand::NACK:

        assure(processId < numSenderProcesses_ && processId >= 0, "No receiver process for this ProcessId");

        senderProcesses_[processId].nackReceived();
    }
}

void
HARQ::calculateSizes(const CommandPool* commandPool, Bit& commandPoolSize, Bit& sduSize) const
{
    //What are the sizes in the upper Layers
    getFUN()->calculateSizes(commandPool, commandPoolSize, sduSize, this);
    //commandPoolSize += checkSumLength;

} // calculateSizes

void
HARQ::addToSendQueue(wns::ldk::CompoundPtr compound)
{
    sendQueue_.push_back(compound);
}


