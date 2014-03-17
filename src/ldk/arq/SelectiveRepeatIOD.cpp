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

#include <ctime>

#include <boost/bind.hpp>
#include <boost/function.hpp>

using namespace wns::ldk;
using namespace wns::ldk::arq;

namespace wns { namespace ldk { namespace arq {

bool const operator==(const timestamp_s n, const timestamp_s &o) {
  return n.time == o.time && n.clock == o.clock;
}

bool const operator<(const timestamp_s n, const timestamp_s &o) {
  return n.time < o.time || (n.time == o.time &&  n.clock < o.clock);
}
}}}

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
    logger(config.get("logger")),
    segmentationBuffer_(logger, windowSize, sequenceNumberSize)
{
  segmentationBuffer_.connectToReassemblySignal(boost::bind(&SelectiveRepeatIOD::onReassembly, this, _1));

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

#warning clonable constructor of SelectiveRepeatIOD is incomplete
SelectiveRepeatIOD::SelectiveRepeatIOD(const SelectiveRepeatIOD& other):
    CommandTypeSpecifier<SelectiveRepeatIODCommand>(other.getFUN()),
    logger(other.logger),
    commandName_(other.commandName_),
    segmentSize_(other.segmentSize_),
    headerSize_(other.headerSize_),
    windowSize(other.windowSize),
    sequenceNumberSize(other.sequenceNumberSize),
    commandSize(other.commandSize),
    sduLengthAddition_(other.sduLengthAddition_),
    nextOutgoingSN_(other.nextOutgoingSN_),
    reorderingWindow_(other.reorderingWindow_),
    reassemblyBuffer_(other.reassemblyBuffer_),
    segmentationBuffer_(other.logger, other.windowSize, other.sequenceNumberSize),
    SuspendSupport(other),
    isSegmenting_(other.isSegmenting_),
    segmentDropRatioCC_(wns::probe::bus::ContextCollectorPtr(
        new wns::probe::bus::ContextCollector(*other.segmentDropRatioCC_))),
    minDelayCC_(wns::probe::bus::ContextCollectorPtr(
        new wns::probe::bus::ContextCollector(*other.minDelayCC_))),
    maxDelayCC_(wns::probe::bus::ContextCollectorPtr(
        new wns::probe::bus::ContextCollector(*other.minDelayCC_))),
    sizeCC_(wns::probe::bus::ContextCollectorPtr(
        new wns::probe::bus::ContextCollector(*other.sizeCC_))),
    probeHeaderReader_(other.probeHeaderReader_)
{
    reorderingWindow_.connectToReassemblySignal(boost::bind(&SelectiveRepeatIOD::onReorderedPDU, this, _1, _2));

    reorderingWindow_.connectToDiscardSignal(boost::bind(&SelectiveRepeatIOD::onDiscardedPDU, this, _1, _2));
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
  segmentationBuffer_.initialize(getFUN()->getCommandReader(commandName_));
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
  SelectiveRepeatIODCommand* command;
  CommandPool* commandPool = compound->getCommandPool();
  command = getCommand(commandPool);

  MESSAGE_BEGIN(VERBOSE, logger, m, "processIncoming: ");
  m << "isSegmented: " << command->isSegmented() << "\tisBegin: " << command->peer.isBegin_ << "\tisEnd: " << command->peer.isEnd_;
  m << "\ttimestamp: " << command->groupId().time;
  MESSAGE_END();

  // treat segmented packet's different
  if(command->isSegmented()){
    // not implemented yet
    segmentationBuffer_.push(compound, command->groupId());
  } else {
    // This sends the complete PDU upwards:
    getDeliverer()->getAcceptor( compound )->onData( compound );
  }
}

void
SelectiveRepeatIOD::processOutgoing(const CompoundPtr& compound)
{
  activeCompound = compound;

  GroupNumber groupId = {time(NULL), clock()};

  Bit sduPCISize = 0;
  Bit sduDataSize = 0;
  Bit sduTotalSize = 0;
  Bit cumSize = 0;
  Bit nextSegmentSize = 0;

  CommandPool* commandPool = compound->getCommandPool();
  getFUN()->calculateSizes(commandPool, sduPCISize, sduDataSize);
  sduTotalSize = sduPCISize + sduDataSize + sduLengthAddition_;

  bool isBegin = true;
  bool isEnd = false;
  
#if 0
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
    // SegAndConcatCommand* command = NULL;

    // wns::ldk::CompoundPtr nextSegment(new wns::ldk::Compound(getFUN()->getProxy()->createCommandPool()));
    // command = activateCommand(nextSegment->getCommandPool());
    // command->setSequenceNumber(nextOutgoingSN_);
    // command->addSDU(sdu->copy());
    // nextOutgoingSN_ += 1;

    // isBegin ? command->setBeginFlag() : command->clearBeginFlag();
    // isEnd ? command->setEndFlag() : command->clearEndFlag();

    // command->increaseDataSize(nextSegmentSize);
    // command->increaseHeaderSize(headerSize_);
    // this->commitSizes(nextSegment->getCommandPool());
    // this->senderPendingSegments_.push_back(nextSegment);

    isBegin = false;
  }
#endif

  CompoundPtr segment  = createStartSegment(compound, nextOutgoingSN_, segmentSize_, groupId);
  nextOutgoingSN_++;
  CompoundPtr segment2 = createSegment(compound, nextOutgoingSN_, segmentSize_, groupId);
  nextOutgoingSN_++;
  CompoundPtr segment3 = createEndSegment(compound, nextOutgoingSN_, segmentSize_, groupId);
  nextOutgoingSN_++;
  MESSAGE_SINGLE(VERBOSE, logger, "processOutgoing: ");

  // command->peer.type = SelectiveRepeatIODCommand::I;

  this->commitSizes(segment3->getCommandPool());
  senderPendingSegments_.push_back(segment);
  senderPendingSegments_.push_back(segment2);
  senderPendingSegments_.push_back(segment3);
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

bool SelectiveRepeatIOD::onReassembly(const CompoundContainer& compoundList)
{
  MESSAGE_SINGLE(VERBOSE, logger, "onReassembly called");
  return true;
}

/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  This is called constantly to poll for packets that need to be sent
 * afterwards packets will be processed by processOutgoing, and subsequently
 *
 *
 * @Returns   CompoundPtr signifying whether or not a packet is available for sending
 */
/* ----------------------------------------------------------------------------*/
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


/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  calculates data size -> dataSize + padding
 *
 * @Param commandPool
 * @Param commandPoolSize
 * @Param dataSize
 */
/* ----------------------------------------------------------------------------*/
void
SelectiveRepeatIOD::calculateSizes(const CommandPool* commandPool,
                                   Bit& commandPoolSize,
                                   Bit& dataSize) const
{
    SelectiveRepeatIODCommand* command;
    command = getCommand(commandPool);

    commandPoolSize = command->peer.headerSize_;
    dataSize = command->peer.dataSize_ + command->peer.paddingSize_;
}

/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  wrappers to create segments, by default the segment is neither a
 *  beginning nor an end segment
 *
 * @Param sdu
 * @Param sequenceNumber
 * @Param segmentSize
 *
 * @Returns   CompoundPtr segment
 */
/* ----------------------------------------------------------------------------*/
CompoundPtr SelectiveRepeatIOD::createSegment(const CompoundPtr& sdu,
                                              long sequenceNumber,
                                              const Bit segmentSize,
                                              GroupNumber groupId){

  return this->createSegment(sdu, sequenceNumber, segmentSize, groupId, false, false);
}

CompoundPtr SelectiveRepeatIOD::createStartSegment(const CompoundPtr& sdu,
                                              long sequenceNumber,
                                              const Bit segmentSize,
                                              GroupNumber groupId){

  return this->createSegment(sdu, sequenceNumber, segmentSize, groupId, true, false);
}

CompoundPtr SelectiveRepeatIOD::createEndSegment(const CompoundPtr& sdu,
                                              long sequenceNumber,
                                              const Bit segmentSize,
                                              GroupNumber groupId){

  return this->createSegment(sdu, sequenceNumber, segmentSize, groupId, false, true);
}

/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  creates a segment when needed
 *
 * @Param sdu
 * @Param sequenceNumber
 * @Param segmentSize
 * @Param isBegin
 * @Param isEnd
 *
 * @Returns   CompoundPtr for segment
 */
/* ----------------------------------------------------------------------------*/
CompoundPtr SelectiveRepeatIOD::createSegment(const CompoundPtr& sdu,
                                              long sequenceNumber,
                                              const Bit segmentSize,
                                              GroupNumber groupId,
                                              bool isBegin,
                                              bool isEnd){
  SelectiveRepeatIODCommand *command = NULL;

  CompoundPtr nextSegment(
      new Compound(
          getFUN()->getProxy()->createCommandPool()));

  command = activateCommand(nextSegment->getCommandPool());
  command->setSequenceNumber(sequenceNumber);
  command->addSDU(sdu->copy());

  // if the segment is both end, and start segment it's not segmented
  if(!(isBegin && isEnd)){
    command->setSegmented();
  }
  command->setGroupId(groupId);

  isBegin ? command->setBeginFlag() : command->clearBeginFlag();
  isEnd ? command->setEndFlag() : command->clearEndFlag();

  command->increaseDataSize(segmentSize);
  command->increaseHeaderSize(headerSize_);

  return nextSegment;
}


