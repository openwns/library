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

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    ChaseCombiningDecoder,
    IDecoder,
    "ChaseCombiningDecoder",
    wns::PyConfigViewCreator);

UniformRandomDecoder::UniformRandomDecoder(const wns::pyconfig::View& config):
    dis_(new wns::distribution::StandardUniform()),
    initialPER_(config.get<double>("initialPER")),
    rolloffFactor_(config.get<double>("rolloffFactor")),
    logger_(config.get("logger"))
{
}

bool
UniformRandomDecoder::canDecode(const wns::scheduler::SchedulingTimeSlotPtr& resourceBlock,
                                const SoftCombiningContainer& softCombiningContainer)
{
    int numTransmissions = 0;

    for (int ii=0; ii < softCombiningContainer.getNumRVs(); ++ii)
    {
        numTransmissions += softCombiningContainer.getEntriesForRV(ii).size();
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

ChaseCombiningDecoder::ChaseCombiningDecoder(const wns::pyconfig::View& config):
    dis_(new wns::distribution::StandardUniform()),
    logger_(config.get("logger"))
{
}

bool
ChaseCombiningDecoder::canDecode(const wns::scheduler::SchedulingTimeSlotPtr& resourceBlock,
                                 const SoftCombiningContainer& softCombiningContainer)
{
    for (int ii=1; ii < softCombiningContainer.getNumRVs(); ++ii)
    {
        assure(softCombiningContainer.getEntriesForRV(ii).size() == 0, "ChaseCombining expects only RV 0 to be used, but RV=" << ii << " is used, too.");
    }
    assure(softCombiningContainer.getEntriesForRV(0).size() > 0, "Chase combining has no receptions for RV 0.");

    std::list<wns::service::phy::power::PowerMeasurementPtr> pms = softCombiningContainer.getEntriesForRV(0);
    std::list<wns::service::phy::power::PowerMeasurementPtr>::iterator it;

    // Calculate effective MIB

    // According to IEEE 802.16m-08/004r5 (PMD) pp. 91 Chase Combinig can be modeled as
    // follows. Sum up all SINR values in the linear domain (power, not dB) for one symbol. From that value
    // determine the Sum Mutual Information per bit (MIB). Calculate the average MIB of all symbols.
    // From this value the PER can be detected according to the coding scheme
    // Here we do not work on symbols but on subchannels. Currently there is only one of
    // that, so the averaging is not necessary.

    double sumSINR = 0.0;
    // For all retransmissions:
    for(it = pms.begin(); it != pms.end(); ++it)
    {
        sumSINR += (*it)->getSINR().get_factor();
    }

    int blocksize = resourceBlock->getNetBlockSizeInBits(); // (net,netto, i.e. without code/redundancy bits)
    double mib = pms.front()->getPhyMode()->getSINR2MIB(wns::Ratio::from_factor(sumSINR));
    double per = pms.front()->getPhyMode()->getMI2PER(mib, blocksize); // MIB not MI is expected here

    MESSAGE_BEGIN(NORMAL, logger_, m, "canDecode: Average MIB=" << mib);
    m << ", bits=" << blocksize;
    m << " => per=" << per;
    MESSAGE_END();

    if ((*dis_)() > per)
    {
        MESSAGE_SINGLE(NORMAL, logger_, "Decode success");
        return true;
    }
    MESSAGE_SINGLE(NORMAL, logger_, "Decode failed");
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
HARQ::storeSchedulingTimeSlot(const wns::scheduler::SchedulingTimeSlotPtr& resourceBlock)
{
    int subChannelIndex = resourceBlock->subChannelIndex;
    int timeSlotIndex   = resourceBlock->timeSlotIndex;
    MESSAGE_SINGLE(NORMAL, logger_, "storeSchedulingTimeSlot("<<subChannelIndex<<"."<<timeSlotIndex<<")");

    if (!resourceBlock->harq.enabled)
    {
        MESSAGE_SINGLE(VERBOSE, logger_, "storeSchedulingTimeSlot("<<subChannelIndex<<"."<<timeSlotIndex<<"): HARQ is disabled. Skipping.");
        return;
    }

    if (resourceBlock->isEmpty())
    {
        MESSAGE_SINGLE(NORMAL, logger_, "storeSchedulingTimeSlot("<<subChannelIndex<<"."<<timeSlotIndex<<"): time slot is empty");
        return;
    }

    if (!resourceBlock->harq.NDI)
    {
        MESSAGE_SINGLE(NORMAL, logger_, "storeSchedulingTimeSlot("<<subChannelIndex<<"."<<timeSlotIndex<<"): Found a retransmission. Skipping.");
        return;
    }
    // at this point we have a brand new transmission which we must take care of...
    // this is the userID seen from here (the sending peer):
    wns::scheduler::UserID userID = resourceBlock->getUserID();

    /**
     * @todo dbn: Check if resourceBlock->getUserID is right in uplink
     */
    if (!harqEntities_.knows(userID))
    {
        MESSAGE_SINGLE(NORMAL, logger_, "Creating new HARQEntity for peer UserID=" << userID->getName() );
        harqEntities_.insert(userID, new HARQEntity(config_.get("harqEntity"), numSenderProcesses_, numReceiverProcesses_, numRVs_, logger_));
    }

    HARQEntity* entity = harqEntities_.find(userID);
    assure(entity->hasCapacity(), "The HARQ Entity for peer UserID=" << userID->getName() << " does not have anymore capacity!");

    // now store the SchedulingTimeSlotPtr
    /**
     * @todo dbn: Need to ask for permission. Use entity->hasCapacity
     */
    entity->newTransmission(resourceBlock);
}

bool
HARQ::canDecode(const wns::scheduler::SchedulingTimeSlotPtr& resourceBlock,
                const wns::service::phy::power::PowerMeasurementPtr& pm)
{
    MESSAGE_SINGLE(VERBOSE, logger_, "onTimeSlotReceived");
    if (!resourceBlock->harq.enabled)
    {
        MESSAGE_SINGLE(VERBOSE, logger_, "onTimeSlotReceived: HARQ is disabled. Skipping.");
        return true;
    }
    // this is the userID seen from the sending peer, so it's myself ;-)
    wns::scheduler::UserID userID = resourceBlock->getUserID();

    if (!harqEntities_.knows(userID))
    {
        MESSAGE_SINGLE(NORMAL, logger_, "Creating new HARQEntity for UserID=" << userID->getName() );
        harqEntities_.insert(userID, new HARQEntity(config_.get("harqEntity"), numSenderProcesses_, numReceiverProcesses_, numRVs_, logger_));
    }
    HARQEntity* entity = harqEntities_.find(userID);
    /**
     * @todo dbn: Need to ask for permission. Use entity->hasCapacity
     */
    return entity->canDecode(resourceBlock, pm);
}

int
HARQ::getNumberOfRetransmissions()
{
    int numberOfRetransmissions=0;
    for (HARQEntityContainer::const_iterator it = harqEntities_.begin();
         it != harqEntities_.end();
         ++it)
    { // foreach user
        numberOfRetransmissions += it->second->getNumberOfRetransmissions();
    }
    return numberOfRetransmissions;
}


wns::scheduler::SchedulingTimeSlotPtr
HARQ::nextRetransmission()
{
    for (HARQEntityContainer::const_iterator it = harqEntities_.begin();
         it != harqEntities_.end();
         ++it)
    { // foreach user
        wns::scheduler::SchedulingTimeSlotPtr r;
        r = it->second->nextRetransmission();
        if (r != NULL)
        {
            return r;
        }
    }
    return wns::scheduler::SchedulingTimeSlotPtr(); // empty
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
HARQEntity::newTransmission(const wns::scheduler::SchedulingTimeSlotPtr& resourceBlock)
{
    if (!resourceBlock->harq.enabled)
    {
        return;
    }

    assure(hasCapacity(), "HARQ Entity is busy but you wanted to start a new transmission!");

    // find free sender process
    for (int ii=0; ii < numSenderProcesses_; ++ii)
    {
        if (senderProcesses_[ii].hasCapacity())
        {
            senderProcesses_[ii].newTransmission(resourceBlock);
            return; // break; // found
        }
    }
    assure(false, "HARQEntity::newTransmission: cannot find free senderProcess");
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
HARQEntity::canDecode(const wns::scheduler::SchedulingTimeSlotPtr& resourceBlock,
                      const wns::service::phy::power::PowerMeasurementPtr& pm)
{
    if (!resourceBlock->harq.enabled)
    {
        return true;
    }

    assure(resourceBlock->harq.processID < receiverProcesses_.size(), "Invalid receiver processID=" << resourceBlock->harq.processID);
    return receiverProcesses_[resourceBlock->harq.processID].canDecode(resourceBlock, pm);
}

void
HARQEntity::enqueueRetransmission(wns::scheduler::SchedulingTimeSlotPtr& resourceBlock)
{
    pendingRetransmissions_.push_back(resourceBlock);
}

int
HARQEntity::getNumberOfRetransmissions()
{
    return pendingRetransmissions_.size();
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
    // ^ why two arguments config.get("decoder") ??
}

bool
HARQReceiverProcess::canDecode(const wns::scheduler::SchedulingTimeSlotPtr& resourceBlock, const wns::service::phy::power::PowerMeasurementPtr& pm)
{
    MESSAGE_BEGIN(NORMAL, logger_, m, "HarqReceiverProcess::receive processID = " << processID_);
    m << ", RV = " << resourceBlock->harq.rv << ", RetryCounter=" << resourceBlock->harq.retryCounter << ", SINR=" << pm->getSINR();
    MESSAGE_END();

    if (resourceBlock->harq.NDI)
    {
        receptionBuffer.clear();
    }

    receptionBuffer.appendEntryForRV(resourceBlock->harq.rv, pm);

    if (decoder_->canDecode(resourceBlock, receptionBuffer))
    {
        resourceBlock->harq.ackCallback();
        return true;
    }
    else
    {
        resourceBlock->harq.nackCallback();
        return false;
    }
}

HARQSenderProcess::HARQSenderProcess(HARQEntity* entity, int processID, int numRVs, wns::logger::Logger logger):
    entity_(entity),
    processID_(processID),
    numRVs_(numRVs),
    logger_(logger),
    resourceBlock_()
{
}

void
HARQSenderProcess::newTransmission(const wns::scheduler::SchedulingTimeSlotPtr& resourceBlock)
{
    assure(resourceBlock_ == NULL, "HARQSender Process " << processID_ << " is busy but you wanted to start a new transmission!");
    resourceBlock_ = resourceBlock;
    resourceBlock->harq.processID = processID_;
    resourceBlock->harq.rv = 0;
    resourceBlock->harq.ackCallback = boost::bind(&HARQSenderProcess::ACK, this);
    resourceBlock->harq.nackCallback = boost::bind(&HARQSenderProcess::NACK, this);
}

bool
HARQSenderProcess::hasCapacity()
{
    return (resourceBlock_ == NULL);
}

void
HARQSenderProcess::ACK()
{
    MESSAGE_SINGLE(NORMAL, logger_, "HARQSender processID=" << processID_ << " received ACK");
    resourceBlock_ = wns::scheduler::SchedulingTimeSlotPtr(); // make empty/free
}

void
HARQSenderProcess::NACK()
{
    //MESSAGE_SINGLE(NORMAL, logger_, "HARQ process " << processID_ << " received NACK");
    MESSAGE_SINGLE(NORMAL, logger_, "HARQSender processID=" << processID_ << " received NACK on resource "<<
                   resourceBlock_->subChannelIndex<<"."<<resourceBlock_->timeSlotIndex);
    resourceBlock_->harq.NDI = false;
    resourceBlock_->harq.retryCounter++;
    entity_->enqueueRetransmission(resourceBlock_);
}
