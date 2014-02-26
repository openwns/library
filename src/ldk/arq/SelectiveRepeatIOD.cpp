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
    FunctionalUnit,
    "wns.arq.SelectiveRepeatIOD",
    FUNConfigCreator);

SelectiveRepeatIOD::SelectiveRepeatIOD(fun::FUN* fuNet, const wns::pyconfig::View& config) :
    CommandTypeSpecifier<SelectiveRepeatIODCommand>(fuNet),
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
  // getFUN()->getCommandReader(commandName_);
    // reassemblyBuffer_.initialize(getFUN()->getCommandReader(commandName_));
}


bool
SelectiveRepeatIOD::hasCapacity() const
{
  return (senderPendingSegments_.empty());
}



const CompoundPtr
SelectiveRepeatIOD::hasACK() const
{
    if(ackPDUs.empty() == false)
    {
        return ackPDUs.front();
    }

    return CompoundPtr();
}


CompoundPtr
SelectiveRepeatIOD::getACK()
{
  assure(hasACK(), getFUN()->getName() + " hasSomethingToSend has not been called to check whether there is something to send.");

  CompoundPtr nextACKToBeSent = ackPDUs.front();
  ackPDUs.pop_front();

  MESSAGE_BEGIN(NORMAL, logger, m, "Sent ACK frame ");
  m << getCommand(nextACKToBeSent->getCommandPool())->getNS();
  MESSAGE_END();

  return nextACKToBeSent;
} // getACK


void
SelectiveRepeatIOD::onTimeout()
{
}

void
SelectiveRepeatIOD::processIncoming(const CompoundPtr& compound)
{
  // This sends the complete PDU upwards:
  getDeliverer()->getAcceptor( compound )->onData( compound );
  CommandPool* commandPool = compound->getCommandPool();

  SelectiveRepeatIODCommand* command;
  command = getCommand(commandPool);
}

void
SelectiveRepeatIOD::processOutgoing(const CompoundPtr& compound)
{
  activeCompound = compound;

  SelectiveRepeatIODCommand* command = activateCommand(compound->getCommandPool());
  this->commitSizes(compound->getCommandPool());

  command->peer.type = SelectiveRepeatIODCommand::I;
  command->setNS(NS);

  ++NS;

  senderPendingSegments_.push_back(compound);
}

void
SelectiveRepeatIOD::onIFrame(const CompoundPtr& compound)
{
} // onIFrame

void
SelectiveRepeatIOD::onACKFrame(const CompoundPtr& compound)
{
} // onACKFrame


void
SelectiveRepeatIOD::prepareRetransmission()
{
} // prepareRetransmission


void
SelectiveRepeatIOD::keepSorted(const CompoundPtr& compound, CompoundContainer& container)
{
} // keepSorted


// remove ACKed PDU from list
void
SelectiveRepeatIOD::removeACKed(const CompoundPtr& ackCompound, CompoundContainer& container)
{
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

/**
 * selctiverepeatiod segmentation and concatenation
 */
void
SelectiveRepeatIOD::onReorderedPDU(long sn, CompoundPtr c)
{
}

void
SelectiveRepeatIOD::onDiscardedPDU(long, CompoundPtr)
{
}

const CompoundPtr
SelectiveRepeatIOD::hasSomethingToSend() const
{
  if (!senderPendingSegments_.empty())
  {
    return senderPendingSegments_.front();
  }
  else
  {
    return CompoundPtr();
  }
}

CompoundPtr
SelectiveRepeatIOD::getSomethingToSend()
{
  assure(hasSomethingToSend(), "getSomethingToSend although nothing to send");
  CompoundPtr compound = senderPendingSegments_.front();

  senderPendingSegments_.pop_front();
  return compound;
}


void
SelectiveRepeatIOD::calculateSizes(const CommandPool* commandPool, Bit& commandPoolSize, Bit& sduSize) const
{
}
