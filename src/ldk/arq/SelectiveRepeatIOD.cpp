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
    statusTimeout_(config.get<double>("statusTimeout")),
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
    nextOutgoingBigSN_(0),
    lastSentSN_(0),
    isSegmenting_(config.get<bool>("isSegmenting")),
    segmentDropRatioProbeName_(config.get<std::string>("segmentDropRatioProbeName")),
    logger(config.get("logger")),
    enableRetransmissions_(config.get<bool>("enableRetransmissions")),
    enableConcatenation_(config.get<bool>("enableConcatenation")),
    enablePadding_(config.get<bool>("enablePadding")),
    dirty_(false),
    segmentationBuffer_(logger, windowSize_, sequenceNumberSize_, enableRetransmissions_)
{
  if (enableConcatenation_) {
    enablePadding_ = true;
  }

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

    timestamp_ = clock();
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
    nextOutgoingBigSN_(other.nextOutgoingBigSN_),
    enableRetransmissions_(other.enableRetransmissions_),
    segmentationBuffer_(other.logger, other.windowSize_, other.sequenceNumberSize_, other.enableRetransmissions_),
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
  Bit paddingSize = 0;

  if(senderPendingSegments_.size() == 1) {
    SelectiveRepeatIODCommand *command = getCommand(senderPendingSegments_.back()->getCommandPool());

    paddingSize = command->paddingSize();
  }

  size_t bufferTotal = senderPendingSegments_.size() + senderPendingStatusSegments_.size() + retransmissionBuffer_.size();
  return ((senderPendingSegments_.empty() || paddingSize > headerSize_)
          && bufferTotal < windowSize_);
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

  MESSAGE_SINGLE(NORMAL, logger, "hasSomethingToSend");
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
  CompoundPtr compound = activeCompound;
  activeCompound = CompoundPtr();

  // if we still have pending status segments we'll just update them
  // to the latest state
  if(!dirty_ || !senderPendingStatusSegments_.empty()){
    return;
  }

  MESSAGE_BEGIN(NORMAL, logger, m, "statusTimeout: ");
  m << "timeout reached";
  MESSAGE_END();
  setNewTimeout(statusTimeout_);
  sendStatus(compound);
}

void
SelectiveRepeatIOD::processIncoming(const CompoundPtr& compound)
{
  SelectiveRepeatIODCommand* command;
  CommandPool* commandPool = compound->getCommandPool();
  command = getCommand(commandPool);

  MESSAGE_BEGIN(VERBOSE, logger, m, "processIncoming: ");
  m << "\tisBegin: " << command->peer.isBegin_ << "\tisEnd: " << command->peer.isEnd_;
  m << "\tbigSN: " << command->bigSN();
  MESSAGE_END();

  const lPdu_t* missingPdus;
  const completedList_t* completedPdus;

  switch(command->peer.type) {
    case SelectiveRepeatIODCommand::I:
      dirty_ = true;
      // treat segmented packet's differently
      segmentationBuffer_.push(compound);


      if(hasTimeoutSet()) {
        cancelTimeout();
      }
      activeCompound = compound;
      setNewTimeout(statusTimeout_);
      break;

    case SelectiveRepeatIODCommand::STATUS:
      completedPdus = command->completedPdus();
      missingPdus = command->missingPdus();

      if (enableRetransmissions_) {
        removeFromOutgoing(completedPdus);
        prepareRetransmission(missingPdus, command->lastRcvdSn());
      }
      break;

    default:
      break;
  }

  if(command->hasPollFlag()){
    sendStatus(compound);
  }
}


void SelectiveRepeatIOD::sendStatus(const CompoundPtr& compound){
  // we don't want to send more than one status segment, and we always
  // want to send the latest status
  if(!senderPendingStatusSegments_.empty()){
    senderPendingStatusSegments_.pop_back();
  }
  CommandPool* commandPool = getFUN()->getProxy()->createReply(compound->getCommandPool(), this);
  CompoundPtr statusCompound = CompoundPtr(new Compound(commandPool));
  SelectiveRepeatIODCommand* statusCommand = activateCommand(commandPool);
  commitSizes(statusCompound->getCommandPool());

  statusCommand->peer.type = SelectiveRepeatIODCommand::STATUS;
  statusCommand->magic.ackSentTime = wns::simulator::getEventScheduler()->getTime();

  if (enableRetransmissions_) {
    segmentationBuffer_.getMissing(statusCommand);
  }

  dirty_ = false;
  senderPendingStatusSegments_.push_back(statusCompound);
}

void
SelectiveRepeatIOD::processOutgoing(const CompoundPtr& compound)
{
  // GroupNumber groupId = {clock(), time(NULL)};
  GroupNumber groupId = wns::simulator::getEventScheduler()->getTime();

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

  CompoundContainer compoundList;

  MESSAGE_BEGIN(NORMAL, logger, m, "processOutgoing: ");
  m << nextOutgoingSN_ << " timestamp: " << groupId;
  m << " id: " << timestamp_;
  MESSAGE_END();
#if 1
  SelectiveRepeatIODCommand *command;
  bool didConcatenate = false;
  Bit segmentSize = segmentSize_ - headerSize_;
  // case 1 no packets exist
  // case 2 last packet still has space and needs padding
  if(enableConcatenation_ == true &&
     !senderPendingSegments_.empty())
  {

    CompoundPtr c = senderPendingSegments_.back();
    command = getCommand(c->getCommandPool());
    Bit size;

    if(command->paddingSize() > headerSize_ ) {
      // whole command fits into the padding
      if (sduTotalSize + headerSize_ < command->paddingSize() ) {
        command->decreasePaddingSize(sduTotalSize + headerSize_);
        command->increaseHeaderSize(headerSize_);
        command->increaseDataSize(sduTotalSize);
        MESSAGE_BEGIN(NORMAL, logger, m, "padding enabled1: ");
        m << sduTotalSize + headerSize_;
        MESSAGE_END();

        sduTotalSize -= sduTotalSize;

        firstSN = command->getSequenceNumber();
        lastSN  = command->getSequenceNumber();
        didConcatenate = true;
        isBegin = false;
      }
      // packet does not fully fit into the previous segment
      else if (command->paddingSize() < sduTotalSize + headerSize_ ) {
        command->increaseHeaderSize(headerSize_);
        command->increaseDataSize(command->paddingSize());

        MESSAGE_BEGIN(NORMAL, logger, m, "padding enabled2: ");
        m << sduTotalSize ;
        m << " paddingSize: " << command->paddingSize();
        m << " id: " << groupId;
        MESSAGE_END();

        sduTotalSize -= command->paddingSize() - headerSize_;
        command->decreasePaddingSize(command->paddingSize());

        firstSN = command->getSequenceNumber();
        didConcatenate = true;
        isBegin = false;
      }
    }
  }
  while(cumSize < sduTotalSize)
  {
    cumSize += segmentSize;
    if (cumSize >= sduTotalSize)
    {
      nextSegmentSize = sduTotalSize - (cumSize - segmentSize);
      isEnd = true;
    }
    else
    {
      nextSegmentSize = segmentSize;
    }

    CompoundPtr nextSegment;

    if(isBegin&&isEnd){
      firstSN = nextOutgoingSN_;
      lastSN = nextOutgoingSN_;
    }
    else if(isBegin){
      firstSN = nextOutgoingSN_;
    }
    else if(isEnd){
      lastSN = nextOutgoingSN_;
    }
    nextSegment  = createSegment(compound, nextSegmentSize, isEnd);
    nextOutgoingSN_ = (nextOutgoingSN_ + 1) % sequenceNumberSize_;
    nextOutgoingBigSN_++;
    compoundList.push_back(nextSegment);
    isBegin = false;
  }

  addToSenderQueue(compoundList, compound, firstSN, lastSN, groupId);

  if(didConcatenate) {
    MESSAGE_BEGIN(NORMAL, logger, m, "concatenating firstSN:");
    m << firstSN;
    m << " lastSN: " << lastSN;
    m << " groupId: " << groupId;
    MESSAGE_END();
    command->addBaseSDU(compound,
                        firstSN,
                        lastSN,
                        groupId);
  }
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
SelectiveRepeatIOD::prepareRetransmission(const lPdu_t* missingPdus, BigSequenceNumber lastRcvdSn)
{
  lPdu_t::const_iterator it;
  const outgoing_by_sn &outsn = outgoingBuffer_.get<pduSN>();

  MESSAGE_BEGIN(VERBOSE, logger, m, "prepare missing ");
  m << missingPdus->size();
  m << " last sent: " << lastSentSN_;
  m << " last received: " << lastRcvdSn;
  MESSAGE_END();

  int gap = lastSentSN_ -  lastRcvdSn;
  if(gap > 0) {
    MESSAGE_BEGIN(NORMAL, logger, m, "missing gap on retransmission side: ");
    m << gap;
    m << " last sent: " << lastSentSN_;
    m << " last received: " << lastRcvdSn;
    MESSAGE_END();
    // add missing pdu list
    for (unsigned int i = 1; i <= gap; i++) {
      outgoing_by_sn::iterator pdu = outsn.find(lastRcvdSn+i);
      assert(pdu != outsn.end());
      retransmissionBuffer_.push_back(pdu->pdu_);
    }
  }

  if(missingPdus->size() == 0) {
    return;
  }

  for (it = missingPdus->begin();
       it != missingPdus->end(); it++){

    // make sure we don't create an entry in the outgoingBuffer with the []
    // operator by mistake, if the entry doesn't exist, the segmented packet
    // was already received
    outgoing_by_sn::iterator pdu = outsn.find(*it);
    if(pdu == outsn.end()) {
      MESSAGE_SINGLE(NORMAL, logger, "missing something wrong");
      continue;
    }

    retransmissionBuffer_.push_back(pdu->pdu_);
  }
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
                                          CompoundPtr compound,
                                          SequenceNumber startSegment,
                                          SequenceNumber endSegment,
                                          GroupNumber timestamp)
{
  CompoundPtr segment;
  SelectiveRepeatIODCommand *command;
  while(!compoundList.empty()){
    segment = compoundList.front();
    command = getCommand(segment->getCommandPool());

    command->addBaseSDU(compound, startSegment, endSegment, timestamp);


    compoundList.pop_front();
    senderPendingSegments_.push_back(segment);

    // MESSAGE_BEGIN(NORMAL, logger, m, "senderQueue");
    // m << " bigsn" << command->bigSN() << " isEnd: " << command->getEndFlag();
    // MESSAGE_END();
  }
}

bool SelectiveRepeatIOD::onReassembly(CompoundPtr compound)
{

  MESSAGE_SINGLE(NORMAL, logger, "reassemble: Passing " << compound->getLengthInBits()
                 << " bits to upper FU.");

  getDeliverer()->getAcceptor( compound )->onData( compound );
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
  if (!retransmissionBuffer_.empty())
  {
    return retransmissionBuffer_.front();
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
  else if (!retransmissionBuffer_.empty()) {
    compound = retransmissionBuffer_.front();
    getCommand(compound->getCommandPool())->localTransmissionCounter++;
    MESSAGE_BEGIN(NORMAL, logger, m, "missing retransmission sent: ");
    m << getCommand(compound->getCommandPool())->bigSN();
    m << " count: " << getCommand(compound->getCommandPool())->localTransmissionCounter;
    MESSAGE_END();
    compound = compound->copy();
    retransmissionBuffer_.pop_front();
  }
  else if (!senderPendingSegments_.empty()) {
    MESSAGE_SINGLE(NORMAL, logger, "transmission count: 0");
    compound = senderPendingSegments_.front();
    SelectiveRepeatIODCommand *command = getCommand(compound->getCommandPool());

    if(enableRetransmissions_) {
      lSegmentCompound_t sdus = command->baseSDUs();
      lSegmentCompound_t::iterator it = sdus.begin();

      for (; it != sdus.end(); ++it)
      {
        outgoingBuffer_.insert(outgoingPdu(it->timestamp,
                                           command->bigSN(),
                                           compound->copy()));
      }
    }
    commitSizes(compound->getCommandPool());
    senderPendingSegments_.pop_front();
    lastSentSN_ = command->bigSN();
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
                                              const Bit segmentSize,
                                              bool isEnd){

  SelectiveRepeatIODCommand *command;

  CompoundPtr nextSegment(
      new Compound(
          getFUN()->getProxy()->createCommandPool()));

  command = activateCommand(nextSegment->getCommandPool());
  command->setSequenceNumber(nextOutgoingSN_);
  command->bigSN(nextOutgoingBigSN_);

  // polling
  if(isEnd || (nextOutgoingBigSN_ % 10)){
    command->setPollFlag();
  }

  command->increaseDataSize(segmentSize);
  command->increaseHeaderSize(headerSize_);

  if(enablePadding_) {
    MESSAGE_BEGIN(NORMAL, logger, m, "padding size: ");
    m <<         segmentSize_ - nextSegment->getLengthInBits();
    MESSAGE_END();
    command->increasePaddingSize(
        segmentSize_ - nextSegment->getLengthInBits());
  }

  return nextSegment;
}


void SelectiveRepeatIOD::removeFromOutgoing(const completedList_t* completedPdus) {
  completedList_t::const_iterator it;
  outgoing_by_timestamp &outts = outgoingBuffer_.get<pduID>();

  for (it = completedPdus->begin();
       it != completedPdus->end();
       ++it)
  {
    outts.erase(*it);
  }
}

bool retransmissionTimeoutReached(const GroupNumber& timeout) {
  bool timedOut = false;
  return timedOut;
}

void SelectiveRepeatIOD::fillRetransmissionBuffer() {
  /*
  mapOutgoing_t::iterator it = outgoingBuffer_.begin();

  for (; it != outgoingBuffer_.end(); ++it)
  {
    // clear if retransmissionTimeout was reached
    if(retransmissionTimeoutReached(it->first)) {
      outgoingBuffer_.erase(it->first);
      continue;
    }

    // if([><]) {

    // }
  }
  */
}
