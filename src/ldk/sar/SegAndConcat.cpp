/******************************************************************************
 * WinProSt Protocol Stack)                                                   *
 * __________________________________________________________________________ *
 *                                                                            *
 * Copyright (C) 2004                                                         *
 * Lehrstuhl f?r Kommunikationsnetze (ComNets)                                *
 * Kopernikusstr. 16, D-52074 Aachen, Germany                                 *
 * phone: ++49-241-80-27910 (phone), fax: ++49-241-80-22242                   *
 * email: msg@comnets.de, www: http://winner.comnets.rwth-aachen.de/~msg      *
 ******************************************************************************/

#include <WNS/ldk/sar/SegAndConcat.hpp>
#include <WNS/service/dll/FlowID.hpp>
#include <WNS/ldk/Layer.hpp>

#include <boost/bind.hpp>
#include <boost/function.hpp>

using namespace wns::ldk::sar;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    SegAndConcat,
    wns::ldk::FunctionalUnit, "wns.sar.SegAndConcat",
    wns::ldk::FUNConfigCreator);

SegAndConcat::SegAndConcat(wns::ldk::fun::FUN* fun,
                                       const wns::pyconfig::View& config):
    wns::ldk::CommandTypeSpecifier<SegAndConcatCommand>(fun),
    logger_(config.get("logger")),
    commandName_(config.get<std::string>("commandName")),
    segmentSize_(config.get<Bit>("segmentSize")),
    headerSize_(config.get<Bit>("headerSize")),
    sduLengthAddition_(config.get<Bit>("sduLengthAddition")),
    nextOutgoingSN_(0),
    reorderingWindow_(config.get("reorderingWindow")),
    isSegmenting_(config.get<bool>("isSegmenting")),
    segmentDropRatioCC_(
        wns::probe::bus::ContextProviderCollection(&fun->getLayer()->getContextProviderCollection()),
        "segmentDropRatio")
{
    reorderingWindow_.connectToReassemblySignal(boost::bind(&SegAndConcat::onReorderedPDU, this, _1, _2));
    reorderingWindow_.connectToDiscardSignal(boost::bind(&SegAndConcat::onDiscardedPDU, this, _1, _2));
}

SegAndConcat::SegAndConcat(const SegAndConcat& other):
    wns::ldk::CommandTypeSpecifier<SegAndConcatCommand>(other.getFUN()),
    logger_(other.logger_),
    commandName_(other.commandName_),
    segmentSize_(other.segmentSize_),
    headerSize_(other.headerSize_),
    sduLengthAddition_(other.sduLengthAddition_),
    nextOutgoingSN_(other.nextOutgoingSN_),
    reorderingWindow_(other.reorderingWindow_),
    reassemblyBuffer_(other.reassemblyBuffer_),
    isSegmenting_(other.isSegmenting_),
    segmentDropRatioCC_(other.segmentDropRatioCC_)
{
    reorderingWindow_.connectToReassemblySignal(boost::bind(&SegAndConcat::onReorderedPDU, this, _1, _2));

    reorderingWindow_.connectToDiscardSignal(boost::bind(&SegAndConcat::onDiscardedPDU, this, _1, _2));
}

SegAndConcat::~SegAndConcat()
{
}

void
SegAndConcat::onFUNCreated()
{
    MESSAGE_SINGLE(NORMAL, logger_, "SegAndConcat::onFUNCreated()");
    reassemblyBuffer_.initialize(getFUN()->getCommandReader(commandName_));
}

void
SegAndConcat::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    wns::ldk::CommandPool* commandPool = compound->getCommandPool();

    SegAndConcatCommand* command;
    command = getCommand(commandPool);

    reorderingWindow_.onSegment(command->peer.sn_, compound);
}

void
SegAndConcat::processOutgoing(const wns::ldk::CompoundPtr& sdu)
{
    if (!isSegmenting_)
    {
        this->senderPendingSegments_.push_back(sdu);
        MESSAGE_SINGLE(NORMAL, logger_, "Adding one SDU with " << sdu->getLengthInBits() 
                << " bits to pending segments. Segmenting disabled.");
        return;
    }

    int sduPCISize = 0;
    int sduDataSize = 0;
    int sduTotalSize = 0;
    int cumSize = 0;
    int nextSegmentSize = 0;

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
        SegAndConcatCommand* command = NULL;

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

void
SegAndConcat::onReorderedPDU(long sn, wns::ldk::CompoundPtr c)
{
    MESSAGE_SINGLE(NORMAL, logger_, "onReorderedPDU(sn=" << sn << "):");
    if (!reassemblyBuffer_.isNextExpectedSegment(c))
    {
        // Segment missing
        MESSAGE_SINGLE(NORMAL, logger_, "onReorderedPDU: PDU " << reassemblyBuffer_.getNextExpectedSN() 
            << " is missing. Clearing reassembly buffer.");

        for(size_t ii=0; ii < reassemblyBuffer_.size(); ++ii)
        {
            segmentDropRatioCC_.put(1.0);
        }
        reassemblyBuffer_.clear();
    }

    if (reassemblyBuffer_.accepts(c))
    {
        MESSAGE_SINGLE(NORMAL, logger_, "onReorderedPDU: Putting PDU " 
            << getCommand(c->getCommandPool())->peer.sn_ << " of size " 
            << getCommand(c->getCommandPool())->totalSize() << " bits into reassembly buffer");
        reassemblyBuffer_.insert(c);
    }
    else
    {
        MESSAGE_SINGLE(NORMAL, logger_, "onReorderedPDU: Dropping PDU " 
            << getCommand(c->getCommandPool())->peer.sn_ << ". isBegin=False.");
    }

    reassembly::ReassemblyBuffer::SegmentContainer sc;
    MESSAGE_SINGLE(VERBOSE, logger_, reassemblyBuffer_.dump());

    int numberOfReassembledSegments = 0;
    sc = reassemblyBuffer_.getReassembledSegments(numberOfReassembledSegments);

    for (int ii=0; ii < numberOfReassembledSegments; ++ii)
    {
        segmentDropRatioCC_.put(0.0);
    }

    MESSAGE_SINGLE(NORMAL, logger_, "reassemble: getReassembledSegments() sc.size()=" << sc.size());

    if (getDeliverer()->size() > 0)
    {
        reassembly::ReassemblyBuffer::SegmentContainer::iterator it;
        for (it=sc.begin(); it!=sc.end(); ++it)
        {
            MESSAGE_SINGLE(NORMAL, logger_, "reassemble: Passing " << (*it)->getLengthInBits() 
                << " bits to upper FU.");
            // This sends the complete PDU upwards:
            getDeliverer()->getAcceptor( (*it) )->onData( (*it) );
        }
    }
    else
    {
        MESSAGE_SINGLE(NORMAL, logger_, "reassemble: No upper FU available.");
    }
}

void
SegAndConcat::onDiscardedPDU(long, wns::ldk::CompoundPtr)
{
    segmentDropRatioCC_.put(1.0);
}

bool
SegAndConcat::hasCapacity() const
{
    return (senderPendingSegments_.empty());
}

const wns::ldk::CompoundPtr
SegAndConcat::hasSomethingToSend() const
{
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
SegAndConcat::getSomethingToSend()
{
    assure(hasSomethingToSend(), "getSomethingToSend although nothing to send");
    wns::ldk::CompoundPtr compound = senderPendingSegments_.front();

    if (isSegmenting_)
    {
        MESSAGE_SINGLE(NORMAL, logger_, "getSomethingToSend: Passing segment " 
            << getCommand(compound->getCommandPool())->peer.sn_ << " of size " 
            << (getCommand(compound->getCommandPool())->totalSize()) << " bits to lower layer");
    }

    senderPendingSegments_.pop_front();
    return compound;
}

void
SegAndConcat::calculateSizes(const wns::ldk::CommandPool* commandPool, Bit& commandPoolSize, Bit& dataSize) const
{
    SegAndConcatCommand* command;
    command = getCommand(commandPool);

    commandPoolSize = command->peer.headerSize_;
    dataSize = command->peer.dataSize_ + command->peer.paddingSize_;
}
