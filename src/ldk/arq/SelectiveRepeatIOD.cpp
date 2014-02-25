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


#include <WNS/ldk/arq/SelectiveRepeatIOD.hpp>
#include <WNS/ldk/Layer.hpp>
#include <WNS/pyconfig/View.hpp>
#include <WNS/Assure.hpp>

#include <boost/bind.hpp>
#include <boost/function.hpp>

using namespace wns::ldk;
using namespace wns::ldk::arq;


STATIC_FACTORY_REGISTER_WITH_CREATOR(
    SelectiveRepeatIOD,
    ARQ,
    "wns.arq.SelectiveRepeatIOD",
    FUNConfigCreator);

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    SelectiveRepeatIOD,
    FunctionalUnit,
    "wns.arq.SelectiveRepeatIOD",
    FUNConfigCreator);

SelectiveRepeatIOD::SelectiveRepeatIOD(fun::FUN* fuNet, const wns::pyconfig::View& config) :
    ARQ(config),

    wns::ldk::fu::Plain<SelectiveRepeatIOD, SelectiveRepeatIODCommand>(fuNet),
    Delayed<SelectiveRepeatIOD>(),
    SuspendSupport(fuNet, config),
    CanTimeout(),

    windowSize(config.get<int>("windowSize")),
    sequenceNumberSize(config.get<int>("sequenceNumberSize")),
    commandSize(config.get<int>("commandSize")),
    NS(0),
    NR(0),
    LA(0),
    activeCompound(CompoundPtr()),
    sentPDUs(),
    toRetransmit(),
    ackPDUs(),
    receivedPDUs(),
    receivedACKs(),
    sendNow(false),
    resendTimeout(config.get<double>("resendTimeout")),
    retransmissionInterval(resendTimeout),
    transmissionAttemptsProbeBus( new wns::probe::bus::ContextCollector(
        wns::probe::bus::ContextProviderCollection(&fuNet->getLayer()->getContextProviderCollection()),
        config.get<std::string>("probeName"))),
    ackDelayProbeBus( new wns::probe::bus::ContextCollector(
        wns::probe::bus::ContextProviderCollection(&fuNet->getLayer()->getContextProviderCollection()),
        config.get<std::string>("ackDelayProbeName"))),
    roundTripTimeProbeBus( new wns::probe::bus::ContextCollector(
        wns::probe::bus::ContextProviderCollection(&fuNet->getLayer()->getContextProviderCollection()),
        config.get<std::string>("RTTProbeName"))),
    delayingDelivery(false),
    commandName_(config.get<std::string>("commandName")),
    segmentSize_(config.get<Bit>("segmentSize")),
    headerSize_(config.get<Bit>("headerSize")),
    sduLengthAddition_(config.get<Bit>("sduLengthAddition")),
    nextOutgoingSN_(0),
    reorderingWindow_(config.get("reorderingWindow")),
    isSegmenting_(config.get<bool>("isSegmenting")),
    segmentDropRatioProbeName_(config.get<std::string>("segmentDropRatioProbeName")),
    logger(config.get("logger"))
{
    reorderingWindow_.connectToReassemblySignal(boost::bind(&SelectiveRepeatIOD::onReorderedPDU, this, _1, _2));
    reorderingWindow_.connectToDiscardSignal(boost::bind(&SelectiveRepeatIOD::onDiscardedPDU, this, _1, _2));

    wns::probe::bus::ContextProviderCollection* cpcParent = &fuNet->getLayer()->getContextProviderCollection();
    wns::probe::bus::ContextProviderCollection cpc(cpcParent);

    segmentDropRatioCC_ = wns::probe::bus::ContextCollectorPtr(
        new wns::probe::bus::ContextCollector(cpc, segmentDropRatioProbeName_));

    if(!config.isNone("delayProbeName"))
    {
        std::string delayProbeName = config.get<std::string>("delayProbeName");
        minDelayCC_ = wns::probe::bus::ContextCollectorPtr(
            new wns::probe::bus::ContextCollector(cpc, 
                delayProbeName + ".minDelay"));
        maxDelayCC_ = wns::probe::bus::ContextCollectorPtr(
            new wns::probe::bus::ContextCollector(cpc, 
                delayProbeName + ".maxDelay"));
        sizeCC_ = wns::probe::bus::ContextCollectorPtr(
            new wns::probe::bus::ContextCollector(cpc, 
                delayProbeName + ".stop.compoundSize"));

        // Same name as the probe prefix
        probeHeaderReader_ = fuNet->getCommandReader(delayProbeName);

        reassemblyBuffer_.enableDelayProbing(minDelayCC_, maxDelayCC_, probeHeaderReader_);
    }

    assure(windowSize >= 2, "Invalid windowSize.");
    assure(sequenceNumberSize >= 2 * windowSize, "Maximum sequence number is to small for chosen windowSize");
}


SelectiveRepeatIOD::~SelectiveRepeatIOD()
{
    // empty internal buffers
    ackPDUs.clear();
    sentPDUs.clear();
    toRetransmit.clear();
    receivedPDUs.clear();
    receivedACKs.clear();
}

void
SelectiveRepeatIOD::onFUNCreated()
{
    MESSAGE_SINGLE(NORMAL, logger, "SelectiveRepeatIOD::onFUNCreated()");
    reassemblyBuffer_.initialize(getFUN()->getCommandReader(commandName_));
}


bool
SelectiveRepeatIOD::hasCapacity() const
{
    /* Make sure we
       1. don't have an active PDU we are processing
       2. aren't occupied with retransmissions
       3. don't exceed the sending window
    */
    return (this->activeCompound == CompoundPtr()
        && this->retransmissionState() == false
        && this->NS - this->LA < this->windowSize);
}



const wns::ldk::CompoundPtr
SelectiveRepeatIOD::hasACK() const
{
    if(ackPDUs.empty() == false)
    {
        return ackPDUs.front();
    }

    return CompoundPtr();
}


const wns::ldk::CompoundPtr
SelectiveRepeatIOD::hasData() const
{
    if(retransmissionState() == true)
    {
        return toRetransmit.front();
    }

    if(activeCompound != CompoundPtr() && sendNow == true)
    {
        return activeCompound;
    }

    return CompoundPtr();
} // hasData


wns::ldk::CompoundPtr
SelectiveRepeatIOD::getACK()
{
    assure(hasACK(), getFUN()->getName() + " hasSomethingToSend has not been called to check whether there is something to send.");

    CompoundPtr nextACKToBeSent = ackPDUs.front();
    ackPDUs.pop_front();

    MESSAGE_BEGIN(NORMAL, logger, m, "Sent ACK frame ");
    m << getCommand(nextACKToBeSent->getCommandPool())->getSequenceNumber();
    MESSAGE_END();

    return nextACKToBeSent;
} // getACK


wns::ldk::CompoundPtr
SelectiveRepeatIOD::getData()
{
    if(retransmissionState() == true)
    {
        assure(
            toRetransmit.empty() == false,
            getFUN()->getName() + " is in retransmission state without anything to retransmit.");

        MESSAGE_BEGIN(NORMAL, logger, m, "Number of Frames to retransmit: ");
        m << toRetransmit.size();
        MESSAGE_END();

        // get first segment to retransmit
        CompoundPtr nextPDUToBeRetransmit = toRetransmit.front();
        // keep track of the number of retransmissions
        SelectiveRepeatIODCommand* command = this->getCommand(nextPDUToBeRetransmit);
        command->localTransmissionCounter++;
        // record the simTime when we made the last attempt to send this compound
        command->local.lastSentTime = wns::simulator::getEventScheduler()->getTime();

        // delete from beginning of retransmission buffer and store it
        // to the end of the sent buffer
        toRetransmit.pop_front();
        sentPDUs.push_back(nextPDUToBeRetransmit);

        MESSAGE_BEGIN(NORMAL, logger, m, "Re-Sent I frame ");
        m << getCommand(nextPDUToBeRetransmit->getCommandPool())->getSequenceNumber();
        MESSAGE_END();

        // print out whether we leave retransmission state
        if(retransmissionState() == false)
        {
            MESSAGE_SINGLE(NORMAL, logger, "Leaving retransmission state");
        }

        // set the Timer
        this->setNewTimeout(resendTimeout);
        // and send a copy
        return nextPDUToBeRetransmit->copy();
    }


    // send a copy
    sendNow = false;
    setNewTimeout(resendTimeout);
    SelectiveRepeatIODCommand* myCommand = this->getCommand(activeCompound);
    // record the simTime when we made the first and the last attempt to send this compound
    myCommand->local.firstSentTime = wns::simulator::getEventScheduler()->getTime();
    myCommand->local.lastSentTime = wns::simulator::getEventScheduler()->getTime();
    // keep track of the number of retransmissions
    myCommand->localTransmissionCounter++;
    CompoundPtr it = activeCompound->copy();
    // store the PDU we now send in the Retransmission FIFO Buffer
    sentPDUs.push_back(activeCompound);
    // empty the space for new outgoing compounds
    activeCompound = CompoundPtr();

    MESSAGE_BEGIN(NORMAL, logger, m, "Sent I frame ");
    m << getCommand(it->getCommandPool())->getSequenceNumber();
    MESSAGE_END();

    return it;
}


void
SelectiveRepeatIOD::onTimeout()
{
    assure(sentPDUs.empty() == false, "No timeout without any sent frames possible");

    MESSAGE_SINGLE(NORMAL, logger, "Entering retransmission state on timeout");

    this->prepareRetransmission();

    // initiate retransmissions
    this->tryToSend();
}

void
SelectiveRepeatIOD::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    wns::ldk::CommandPool* commandPool = compound->getCommandPool();

    SelectiveRepeatIODCommand* command;
    command = getCommand(commandPool);

    reorderingWindow_.onSegment(command->peer.sn_, compound);
}

void
SelectiveRepeatIOD::processOutgoing(const wns::ldk::CompoundPtr& sdu)
{
    if (!isSegmenting_)
    {
        this->senderPendingSegments_.push_back(sdu);
        MESSAGE_SINGLE(NORMAL, logger, "Adding one SDU with " << sdu->getLengthInBits() 
                << " bits to pending segments. Segmenting disabled.");
        return;
    }

    Bit sduPCISize = 0;
    Bit sduDataSize = 0;
    Bit sduTotalSize = 0;
    Bit cumSize = 0;
    Bit nextSegmentSize = 0;

    wns::ldk::CommandPool* commandPool = sdu->getCommandPool();
    getFUN()->calculateSizes(commandPool, sduPCISize, sduDataSize);
    sduTotalSize = sduPCISize + sduDataSize + sduLengthAddition_;

    bool isBegin = true;
    bool isEnd = false;

    while(cumSize < sduTotalSize)
    {
        cumSize += segmentSize_;
        if (cumSize >= sduTotalSize)
        {
            nextSegmentSize = sduTotalSize - (cumSize - segmentSize_);
            isEnd = true;
        }
        else
        {
            nextSegmentSize = segmentSize_;
        }

        // Prepare segment
        SelectiveRepeatIODCommand* command = NULL;

        wns::ldk::CompoundPtr nextSegment(new wns::ldk::Compound(getFUN()->getProxy()->createCommandPool()));
        command = activateCommand(nextSegment->getCommandPool());
        command->setSequenceNumber(nextOutgoingSN_);
        command->addSDU(sdu->copy());
        nextOutgoingSN_ += 1;

        isBegin ? command->setBeginFlag() : command->clearBeginFlag();
        isEnd ? command->setEndFlag() : command->clearEndFlag();

        command->increaseDataSize(nextSegmentSize);
        command->increaseHeaderSize(headerSize_);
        this->commitSizes(nextSegment->getCommandPool());
        this->senderPendingSegments_.push_back(nextSegment);

        isBegin = false;
    }
}

#if 0
void
SelectiveRepeatIOD::processIncoming(const CompoundPtr& compound)
{
    SelectiveRepeatIODCommand *command = this->getCommand(compound);

    switch(command->peer.type)
    {
    case SelectiveRepeatIODCommand::I:
    {
        this->onIFrame(compound);
        break;
    }

    case SelectiveRepeatIODCommand::ACK:
    {
        this->onACKFrame(compound);
        break;
    }
    }
}

void
SelectiveRepeatIOD::processOutgoing(const CompoundPtr& compound)
{
    assure(hasCapacity(), "processOutgoing called although not accepting.");
    activeCompound = compound;

    SelectiveRepeatIODCommand* command = activateCommand(compound->getCommandPool());
    this->commitSizes(compound->getCommandPool());

    command->peer.type = SelectiveRepeatIODCommand::I;
    command->setSequenceNumber(NS);

    MESSAGE_BEGIN(NORMAL, logger, m,  "processOutgoing NS -> ");
    m << command->getSequenceNumber();
    MESSAGE_END();

    ++NS;

    sendNow = true;
}
#endif

void
SelectiveRepeatIOD::onIFrame(const CompoundPtr& compound)
{
    SelectiveRepeatIODCommand* command = this->getCommand(compound);

    MESSAGE_BEGIN(NORMAL, logger, m, "Received I frame.");
    m << " expected " << NR
      << " received " << command->getSequenceNumber();
    MESSAGE_END();

    if(command->getSequenceNumber() == NR && delayingDelivery == false)
    {
        // this is the I frame we waited for.
        MESSAGE_BEGIN(NORMAL, logger, m, "Delivering I frame ");
        m << NR;
        MESSAGE_END();

        this->getDeliverer()->getAcceptor(compound)->onData(compound);
        ++NR;

        // check if there are subsequent frames we have already received
        while (receivedPDUs.empty() == false)
        {
            ARQCommand::SequenceNumber toDeliver =  getCommand(receivedPDUs.front())->getSequenceNumber();

            if(toDeliver != NR)
            {
                break;
            }

            assure(toDeliver == NR,
                   "NS must be equal to NR since the loop will not be entered if it's greater zero and it cannot be smaller!!");
            // if so, deliver them
            MESSAGE_BEGIN(NORMAL, logger, m, "Delivering I frame ");
            m << NR;
            MESSAGE_END();

            getDeliverer()->getAcceptor(receivedPDUs.front())->onData(receivedPDUs.front());

            // and remove them from the receivedPDUs list.
            receivedPDUs.pop_front();
            MESSAGE_BEGIN(NORMAL, logger, m, "Removing from receivedPDUs: I-Frame ");
            m << NR;
            MESSAGE_END();

            // adjust received PDU counter
            ++NR;
            }

    }
    else
    {
        // we received an out-of-sequence frame
        if(command->getSequenceNumber() > NR)
        {
            MESSAGE_BEGIN(NORMAL, logger, m,"Buffering out-of-sequence I frame ");
            m << command->getSequenceNumber();
            MESSAGE_END();

            // store the received frame for later
            keepSorted(compound, receivedPDUs);
        }
        else
        {
            /* we received an old frame (ACK got lost)
               re-send the ACK and discard the frame */
        }
    }

    if (delayingDelivery == false || command->getSequenceNumber() > NR)
    {
        // acknowledge the received I-Frame
        CommandPool* ackPCI = getFUN()->getProxy()->createReply(compound->getCommandPool(), this);
        CompoundPtr ack = CompoundPtr(new Compound(ackPCI));
        ackPDUs.push_back(ack);
        SelectiveRepeatIODCommand* ackCommand = activateCommand(ackPCI);
        this->commitSizes(ack->getCommandPool());

        ackCommand->peer.type = SelectiveRepeatIODCommand::ACK;
        ackCommand->setNS(  getCommand(compound)->getSequenceNumber() );
        ackCommand->magic.ackSentTime = wns::simulator::getEventScheduler()->getTime();

        MESSAGE_BEGIN(NORMAL, logger, m, "Number of ACKs pending: ");
        m << ackPDUs.size();
        MESSAGE_END();
    }

} // onIFrame

void
SelectiveRepeatIOD::onACKFrame(const CompoundPtr& compound)
{
    SelectiveRepeatIODCommand* command = this->getCommand(compound);

    // Delete ACKed frame from one of the retransmission buffers
    // it may happen, that due to duplicate ACKs the PDU is neither in sentPDUs
    // nor in toRetransmit
    this->removeACKed(compound, this->sentPDUs);
    this->removeACKed(compound, this->toRetransmit);

    if(command->getSequenceNumber() == this->LA )
    {
        // received the expected ACK
        MESSAGE_BEGIN(NORMAL, this->logger, m, "Received ACK frame.");
        m << " expected " << this->LA << " received " << command->getSequenceNumber();
        MESSAGE_END();
        // adjust counter (advance sending window)
        this->LA++;
        // probe the time this ACK took to travel back to me
        this->ackDelayProbeBus->put( compound, wns::simulator::getEventScheduler()->getTime() - command->magic.ackSentTime );

        // Now check if subsequent ACKs have been received before
        while(this->receivedACKs.empty() == false)
        {
            if(this->getCommand(this->receivedACKs.front())->getSequenceNumber() == this->LA)
            {
                this->LA++;
                this->receivedACKs.front() = CompoundPtr();
                this->receivedACKs.pop_front();
            }
            else
            {
                break;
            }
        }
        this->trySuspend();
    }
    else
    {
        //received out-of-sequence ACK
        MESSAGE_BEGIN(NORMAL, logger, m, "Received Out-of-sequence ACK frame");
        m << " expected " << this->LA << " received " << command->getSequenceNumber();
        MESSAGE_END();

        // probe the time this ACK took to travel back to me
        this->ackDelayProbeBus->put( compound, wns::simulator::getEventScheduler()->getTime() - command->magic.ackSentTime );

        if (command->getSequenceNumber() > this->LA)
        {
            // Enter Retransmission State
            MESSAGE_SINGLE(NORMAL, logger,"Entering retransmission state on out-of-sequence ACK");
            // push current ACK to the list of received ones
            this->keepSorted(compound, this->receivedACKs);
            // prepare PDU List for Retransmission
            this->prepareRetransmission();
            if (this->retransmissionState() == false)
            {
                MESSAGE_SINGLE(NORMAL, logger,"Leaving retransmission state, no Retransmissions pending.");
            }
        }
        else
        {
            MESSAGE_SINGLE(NORMAL, logger, "ACK is a duplicate, discarding ...");
        }
    }

    if (this->sentPDUs.empty() == true && this->hasTimeoutSet() == true)
    {
        this->cancelTimeout();
    }
    // Take care of pending retransmissions, if any
    if (this->retransmissionState())
    {
        this->tryToSend();
    }
} // onACKFrame


void
SelectiveRepeatIOD::prepareRetransmission()
{
    bool retransmitAll = false;
    ARQCommand::SequenceNumber lastACK(0);
    if (receivedACKs.empty() == true)
    {
        // no ACKs received so far
        retransmitAll = true;
    }
    else
    {
        // remember the last ACK
        lastACK = this->getCommand(receivedACKs.back())->getSequenceNumber();
    }

    for (CompoundContainer::iterator it = sentPDUs.begin(); it != sentPDUs.end();	)
    {
        SelectiveRepeatIODCommand* command = this->getCommand(*it);
        ARQCommand::SequenceNumber lookingAt = command->getSequenceNumber();
        if ( lookingAt < lastACK || retransmitAll )
        {
            if (command->local.lastSentTime+retransmissionInterval <= wns::simulator::getEventScheduler()->getTime()
                ||
                command->local.lastSentTime==command->local.firstSentTime)
            {
                MESSAGE_BEGIN(NORMAL, logger, m,  "Chosing I-Frame ");
                m << lookingAt << " for retransmission:";
                MESSAGE_END();

                CompoundContainer::iterator copyOfIt = it++;
                toRetransmit.push_back((*copyOfIt));

                // collect statistics available for other FUs
                this->statusCollector->onFailedTransmission((*copyOfIt));

                sentPDUs.erase(copyOfIt);
                continue;
            }
        }
        ++it;
    }
} // prepareRetransmission


void
SelectiveRepeatIOD::keepSorted(const CompoundPtr& compound, CompoundContainer& container)
{
    SelectiveRepeatIODCommand* command = this->getCommand(compound);

    ARQCommand::SequenceNumber toInsert = command->getSequenceNumber();
    CompoundContainer::iterator it;
    CompoundContainer::iterator itEnd = container.end();

    for(it = container.begin(); it != itEnd; ++it)
    {
        ARQCommand::SequenceNumber lookingAt = getCommand(*it)->getSequenceNumber();

        if(lookingAt == toInsert)
        {
            MESSAGE_SINGLE(NORMAL, logger, "Don't need to insert, already in list");
            return;
        }

        if(lookingAt > toInsert)
        {
            break;
        }
    }
    container.insert(it, compound);
} // keepSorted


// remove ACKed PDU from list
void
SelectiveRepeatIOD::removeACKed(const CompoundPtr& ackCompound, CompoundContainer& container)
{
    SelectiveRepeatIODCommand* command = getCommand(ackCompound->getCommandPool());
    int NS = command->getSequenceNumber();
    for(CompoundContainer::iterator iter = container.begin();  iter != container.end(); ++iter )
    {
        if(getCommand((*iter)->getCommandPool())->getSequenceNumber() == NS)
        {
            // a probe counting the number of transmissions needed
            transmissionAttemptsProbeBus->put( ackCompound, getCommand(*iter)->localTransmissionCounter);
            // a probe counting the RoundTripTime needed
            simTimeType rtt = wns::simulator::getEventScheduler()->getTime() - getCommand((*iter)->getCommandPool())->local.firstSentTime;
            roundTripTimeProbeBus->put(ackCompound, rtt);
            // adjust min time between retransmissions to two times the RTT
            retransmissionInterval = std::min<simTimeType>(2*rtt, retransmissionInterval);

            // collect statistics available for other FUs
            this->statusCollector->onSuccessfullTransmission((*iter));

            MESSAGE_BEGIN(NORMAL, logger, m, "ACK frame received after ");
            m << rtt << " s and " << getCommand(*iter)->localTransmissionCounter << " transmission attempts. RTI is now " << retransmissionInterval;
            MESSAGE_END();
            container.erase(iter);
            break;
        }
    }
}

// return whether we are in retransmission mode
bool
SelectiveRepeatIOD::retransmissionState() const
{
    return !toRetransmit.empty();
}

bool
SelectiveRepeatIOD::onSuspend() const
{
    return NS == LA;
} // onSuspend

void
SelectiveRepeatIOD::doDelayDelivery()
{
    delayingDelivery = true;
} // doDelayDelivery

void
SelectiveRepeatIOD::doDeliver()
{
    delayingDelivery = false;

    // check if there are subsequent frames we have already received
    while (!receivedPDUs.empty())
    {
        ARQCommand::SequenceNumber toDeliver =  getCommand(receivedPDUs.front())->getSequenceNumber();

        if(toDeliver != NR)
        {
            break;
        }

        assure(toDeliver == NR,
               "NS must be equal to NR since the loop will not be entered if it's greater zero and it cannot be smaller!!");
        // if so, deliver them
        MESSAGE_BEGIN(NORMAL, logger, m, "Delivering I frame ");
        m << NR;
        MESSAGE_END();

        getDeliverer()->getAcceptor(receivedPDUs.front())->onData(receivedPDUs.front());

        // and remove them from the receivedPDUs list.
        receivedPDUs.pop_front();
        MESSAGE_BEGIN(NORMAL, logger, m, "Removing from receivedPDUs: I-Frame ");
        m << NR;
        MESSAGE_END();

        // adjust received PDU counter
        ++NR;
    }

} // doDeliver


/**
 * selctiverepeatiod segmentation and concatenation
 */
void
SelectiveRepeatIOD::onReorderedPDU(long sn, wns::ldk::CompoundPtr c)
{
    MESSAGE_SINGLE(NORMAL, logger, "onReorderedPDU(sn=" << sn << "):");
    if (!reassemblyBuffer_.isNextExpectedSegment(c))
    {
        // Segment missing
        MESSAGE_SINGLE(NORMAL, logger, "onReorderedPDU: PDU " << reassemblyBuffer_.getNextExpectedSN() 
            << " is missing. Clearing reassembly buffer.");

        for(size_t ii=0; ii < reassemblyBuffer_.size(); ++ii)
        {
            segmentDropRatioCC_->put(1.0);
        }
        reassemblyBuffer_.clear();
    }

    if (reassemblyBuffer_.accepts(c))
    {
        MESSAGE_SINGLE(NORMAL, logger, "onReorderedPDU: Putting PDU " 
            << getCommand(c->getCommandPool())->peer.sn_ << " of size " 
            << getCommand(c->getCommandPool())->totalSize() << " bits into reassembly buffer");
        reassemblyBuffer_.insert(c);
    }
    else
    {
        MESSAGE_SINGLE(NORMAL, logger, "onReorderedPDU: Dropping PDU " 
            << getCommand(c->getCommandPool())->peer.sn_ << ". isBegin=False.");
    }

    sar::reassembly::ReassemblyBuffer::SegmentContainer sc;
    MESSAGE_SINGLE(VERBOSE, logger, reassemblyBuffer_.dump());

    int numberOfReassembledSegments = 0;
    sc = reassemblyBuffer_.getReassembledSegments(numberOfReassembledSegments);

    for (int ii=0; ii < numberOfReassembledSegments; ++ii)
    {
        segmentDropRatioCC_->put(0.0);
    }

    MESSAGE_SINGLE(NORMAL, logger, "reassemble: getReassembledSegments() sc.size()=" << sc.size());

    if (getDeliverer()->size() > 0)
    {
        sar::reassembly::ReassemblyBuffer::SegmentContainer::iterator it;
        for (it=sc.begin(); it!=sc.end(); ++it)
        {
            MESSAGE_SINGLE(NORMAL, logger, "reassemble: Passing " << (*it)->getLengthInBits() 
                << " bits to upper FU.");
            // This sends the complete PDU upwards:
            getDeliverer()->getAcceptor( (*it) )->onData( (*it) );
            if(sizeCC_ != NULL)
                sizeCC_->put((*it)->getLengthInBits());
        }
    }
    else
    {
        MESSAGE_SINGLE(NORMAL, logger, "reassemble: No upper FU available.");
    }
}

void
SelectiveRepeatIOD::onDiscardedPDU(long, wns::ldk::CompoundPtr)
{
    segmentDropRatioCC_->put(1.0);
}

/**
 * TODO: remove old calculateSizes
 */
#if 0
void
SelectiveRepeatIOD::calculateSizes(const CommandPool* commandPool, Bit& commandPoolSize, Bit& sduSize) const
{
    //What are the sizes in the upper Layers
    getFUN()->calculateSizes(commandPool, commandPoolSize, sduSize, this);


    MESSAGE_SINGLE(VERBOSE, logger, "Size calc - Command: "<<commandPoolSize<<" Payload: "<<sduSize);

    MESSAGE_SINGLE(VERBOSE, logger, "Size of SR-ARQ Command: "<<commandSize<<" Bit");

    commandPoolSize += commandSize;

    MESSAGE_SINGLE(VERBOSE, logger, "Size calc - Command: "<<commandPoolSize<<" Payload: "<<sduSize);
} // calculateSizes

#else

const wns::ldk::CompoundPtr
SelectiveRepeatIOD::hasSomethingToSend() const
{
    MESSAGE_SINGLE(VERBOSE, logger, "we have nothing to send");

    if (!senderPendingSegments_.empty())
    {
        return senderPendingSegments_.front();
    }
    else
    {
        return wns::ldk::CompoundPtr();
    }
}

wns::ldk::CompoundPtr
SelectiveRepeatIOD::getSomethingToSend()
{
    assure(hasSomethingToSend(), "getSomethingToSend although nothing to send");
    wns::ldk::CompoundPtr compound = senderPendingSegments_.front();

    if (isSegmenting_)
    {
        MESSAGE_SINGLE(NORMAL, logger, "getSomethingToSend: Passing segment " 
            << getCommand(compound->getCommandPool())->peer.sn_ << " of size " 
            << (getCommand(compound->getCommandPool())->totalSize()) << " bits to lower layer");
    }

    senderPendingSegments_.pop_front();
    return compound;
}


void
SelectiveRepeatIOD::calculateSizes(const wns::ldk::CommandPool* commandPool, Bit& commandPoolSize, Bit& sduSize) const
{
    SelectiveRepeatIODCommand* command;
    command = getCommand(commandPool);

    MESSAGE_SINGLE(VERBOSE, logger, "Size calc - Command: "<<commandPoolSize<<" Payload: "<<sduSize);

    MESSAGE_SINGLE(VERBOSE, logger, "Size of SR-ARQ Command: "<<commandSize<<" Bit");


    commandPoolSize = command->peer.headerSize_;
    sduSize = command->peer.dataSize_ + command->peer.paddingSize_;

    MESSAGE_SINGLE(VERBOSE, logger, "Size calc - Command: "<<commandPoolSize<<" Payload: "<<sduSize);
}
#endif
