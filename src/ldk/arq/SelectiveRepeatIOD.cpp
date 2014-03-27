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

    windowSize_(config.get<int>("windowSize")),
    sequenceNumberSize_(config.get<int>("sequenceNumberSize")),
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
    resendTimeout(config.get<double>("resendTimeout")),
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
    isSegmenting_(config.get<bool>("isSegmenting")),
    segmentDropRatioProbeName_(config.get<std::string>("segmentDropRatioProbeName")),
    logger(config.get("logger")),
    segmentationBuffer_(logger, windowSize_, sequenceNumberSize_)
{
  segmentationBuffer_.connectToReassemblySignal(boost::bind(&SelectiveRepeatIOD::onReassembly, this, _1));

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

    }

    assure(windowSize_ >= 2, "Invalid windowSize.");
    assure(sequenceNumberSize_ >= 2 * windowSize_, "Maximum sequence number is to small for chosen windowSize");

}

#warning clonable constructor of SelectiveRepeatIOD is incomplete
SelectiveRepeatIOD::SelectiveRepeatIOD(const SelectiveRepeatIOD& other):
    CommandTypeSpecifier<SelectiveRepeatIODCommand>(other.getFUN()),
    logger(other.logger),
    commandName_(other.commandName_),
    segmentSize_(other.segmentSize_),
    headerSize_(other.headerSize_),
    windowSize_(other.windowSize_),
    sequenceNumberSize_(other.sequenceNumberSize_),
    commandSize(other.commandSize),
    sduLengthAddition_(other.sduLengthAddition_),
    nextOutgoingSN_(other.nextOutgoingSN_),
    segmentationBuffer_(other.logger, other.windowSize_, other.sequenceNumberSize_),
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

  MESSAGE_BEGIN(NORMAL, logger, m, "processIncoming: ");
  m << "isSegmented: " << command->isSegmented() << "\tisBegin: " << command->peer.isBegin_ << "\tisEnd: " << command->peer.isEnd_;
  m << "\ttimestamp: " << command->groupId().time;
  MESSAGE_END();

  if (command->isStatus()){
    MESSAGE_SINGLE(VERBOSE, logger, "Parsing status command");
  }
  else if(command->isSegmented()){
    // treat segmented packet's differently
    segmentationBuffer_.push(compound, command->groupId());
  } else {
    // This sends the complete PDU to the upper layer
    getDeliverer()->getAcceptor( compound )->onData( compound );
  }

  if(command->hasPollFlag()){
    sendPoll(compound);
  }
}

void SelectiveRepeatIOD::sendPoll(const CompoundPtr& compound){
  CommandPool* commandPool = getFUN()->getProxy()->createReply(compound->getCommandPool(), this);
  CompoundPtr statusCompound = CompoundPtr(new Compound(commandPool));
  SelectiveRepeatIODCommand* statusCommand = activateCommand(commandPool);
  commitSizes(statusCompound->getCommandPool());

  statusCommand->peer.type = SelectiveRepeatIODCommand::STATUS;
  statusCommand->magic.ackSentTime = wns::simulator::getEventScheduler()->getTime();

  senderPendingStatusSegments_.push_back(statusCompound);
}

void
SelectiveRepeatIOD::processOutgoing(const CompoundPtr& compound)
{
  activeCompound = compound;

  GroupNumber groupId = {clock(), time(NULL)};

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

  SequenceNumber firstSN = 0;
  SequenceNumber lastSN = 0;

  CompoundContainer outgoingBuffer;

  MESSAGE_BEGIN(NORMAL, logger, m, "processOutgoing: ");
  m << nextOutgoingSN_;
  MESSAGE_END();
#if 1
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

    CompoundPtr nextSegment;

    if(isBegin){
      firstSN = nextOutgoingSN_;
      nextSegment  = createStartSegment(compound, nextOutgoingSN_, nextSegmentSize, groupId);
    }
    else if(isEnd){
      lastSN = nextOutgoingSN_;
      nextSegment  = createEndSegment(compound, nextOutgoingSN_, nextSegmentSize, groupId);
    }
    else {
      nextSegment  = createSegment(compound, nextOutgoingSN_, nextSegmentSize, groupId);
    }
    nextOutgoingSN_ = (nextOutgoingSN_ + 1) % sequenceNumberSize_;
    outgoingBuffer.push_back(nextSegment);
    commitSizes(nextSegment->getCommandPool());
    isBegin = false;
  }
  addToSenderQueue(outgoingBuffer, firstSN, lastSN);
#else
  GroupNumber groupId2 = {clock(), time(NULL)};

  CompoundPtr segment = createStartSegment(compound,
                                           nextOutgoingSN_,
                                           segmentSize_,
                                           groupId);
  nextOutgoingSN_++;
  CompoundPtr segment4 = createStartSegment(compound,
                                           nextOutgoingSN_+2,
                                           segmentSize_,
                                           groupId2);
  CompoundPtr segment5 = createEndSegment(compound,
                                          nextOutgoingSN_+3,
                                          segmentSize_,
                                          groupId2);
  CompoundPtr segment2 = createSegment(compound, nextOutgoingSN_, segmentSize_, groupId);
  nextOutgoingSN_++;
  CompoundPtr segment3 = createEndSegment(compound, nextOutgoingSN_, segmentSize_, groupId);
  nextOutgoingSN_++;
  nextOutgoingSN_++;
  nextOutgoingSN_++;

  // command->peer.type = SelectiveRepeatIODCommand::I;

  commitSizes(segment4->getCommandPool());
  senderPendingSegments_.push_back(segment);
  senderPendingSegments_.push_back(segment4);
  senderPendingSegments_.push_back(segment2);
  senderPendingSegments_.push_back(segment5);
  senderPendingSegments_.push_back(segment3);
#endif
}

void
SelectiveRepeatIOD::prepareRetransmission()
{
} // prepareRetransmission


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


/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  before adding to the sender queue, we add the start and end SN
 * of every item in this sequence to the magic part of the header
 *
 * This allows quick checking which segments are missing
 *
 * @Param compoundList
 * @Param startSegment
 * @Param endSegment
 */
/* ----------------------------------------------------------------------------*/
void SelectiveRepeatIOD::addToSenderQueue(CompoundContainer& compoundList,
                                          SequenceNumber startSegment,
                                          SequenceNumber endSegment)
{
  CompoundPtr compound;
  CommandPool* commandPool;
  SelectiveRepeatIODCommand *command;
  while(!compoundList.empty()){
    compound = compoundList.front();
    commandPool  = compound->getCommandPool();
    command = getCommand(commandPool);

    command->setStartSN(startSegment);
    command->setEndSN(endSegment);

    compoundList.pop_front();
    senderPendingSegments_.push_back(compound);
  }
}

bool SelectiveRepeatIOD::onReassembly(const compoundReassembly_t& compoundList)
{
  CompoundPtr compound = compoundList.begin()->second;
  SelectiveRepeatIODCommand *command = getCommand(compound);

  MESSAGE_SINGLE(NORMAL, logger, "reassemble: Passing " << command->baseSDU()->getLengthInBits()
                 << " bits to upper FU.");

  getDeliverer()->getAcceptor( command->baseSDU() )->onData( command->baseSDU() );
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
  if (!senderPendingStatusSegments_.empty())
  {
    return senderPendingStatusSegments_.front();
  }
  else if (!senderPendingSegments_.empty())
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
  CompoundPtr compound;
  if (!senderPendingStatusSegments_.empty()){
    compound = senderPendingStatusSegments_.front();
    senderPendingStatusSegments_.pop_front();
  }
  else {
    compound = senderPendingSegments_.front();
    senderPendingSegments_.pop_front();
  }

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
                                              SequenceNumber sequenceNumber,
                                              const Bit segmentSize,
                                              GroupNumber groupId){

  return createSegment(sdu, sequenceNumber, segmentSize, groupId, false, false);
}

CompoundPtr SelectiveRepeatIOD::createStartSegment(const CompoundPtr& sdu,
                                              SequenceNumber sequenceNumber,
                                              const Bit segmentSize,
                                              GroupNumber groupId){

  return createSegment(sdu, sequenceNumber, segmentSize, groupId, true, false);
}

CompoundPtr SelectiveRepeatIOD::createEndSegment(const CompoundPtr& sdu,
                                              SequenceNumber sequenceNumber,
                                              const Bit segmentSize,
                                              GroupNumber groupId){

  return createSegment(sdu, sequenceNumber, segmentSize, groupId, false, true);
}

CompoundPtr SelectiveRepeatIOD::createUnsegmented(const CompoundPtr& sdu,
                                                  SequenceNumber sequenceNumber,
                                                  const Bit segmentSize,
                                                  GroupNumber groupId){

  return createSegment(sdu, sequenceNumber, segmentSize, groupId, true, true);
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
                                              SequenceNumber sequenceNumber,
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
  // command->addSDU(sdu->copy());
  command->setBaseSDU(sdu);

  // if the segment is both end, and start segment it's not segmented
  if(!(isBegin && isEnd)){
    command->setSegmented();
  }
  command->setGroupId(groupId);
  command->clearEndFlag();

  isBegin ? command->setBeginFlag() : command->clearBeginFlag();
  if(isEnd){
    command->setEndFlag();
    command->setPollFlag();
  }

  command->increaseDataSize(segmentSize);
  command->increaseHeaderSize(headerSize_);

  return nextSegment;
}


