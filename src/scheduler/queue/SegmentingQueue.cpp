/*******************************************************************************
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2004-2009
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

#include <WNS/scheduler/queue/SegmentingQueue.hpp>
#include <WNS/probe/bus/utils.hpp>
#include <WNS/ldk/Layer.hpp>
#include <WNS/ldk/Compound.hpp>

using namespace wns::scheduler;
using namespace wns::scheduler::queue;


STATIC_FACTORY_REGISTER_WITH_CREATOR(SegmentingQueue,
                                     QueueInterface,
                                     "SegmentingQueue",
                                     wns::HasReceptorConfigCreator);

SegmentingQueue::SegmentingQueue(wns::ldk::HasReceptorInterface*, const wns::pyconfig::View& _config)
    : probeContextProviderForCid(NULL),
      probeContextProviderForPriority(NULL),
      segmentHeaderReader(NULL),
      logger(_config.get("logger")),
      config(_config),
      myFUN(),
      maxSize(0),
      minimumSegmentSize(_config.get<uint32_t>("minimumSegmentSize")),
      fixedHeaderSize(_config.get<Bit>("fixedHeaderSize")),
      extensionHeaderSize(_config.get<Bit>("extensionHeaderSize")),
      usePadding(_config.get<bool>("usePadding"))
{
}

SegmentingQueue::~SegmentingQueue()
{
    if (probeContextProviderForCid) { delete probeContextProviderForCid; }
    if (probeContextProviderForPriority) { delete probeContextProviderForPriority; }
}

void SegmentingQueue::setFUN(wns::ldk::fun::FUN* fun)
{
    myFUN = fun;
    // read the localIDs from the config
    wns::probe::bus::ContextProviderCollection localContext(&fun->getLayer()->getContextProviderCollection());
    for (int ii = 0; ii<config.len("localIDs.keys()"); ++ii)
    {
        std::string key = config.get<std::string>("localIDs.keys()",ii);
        uint32_t value  = config.get<uint32_t>("localIDs.values()",ii);
        localContext.addProvider( wns::probe::bus::contextprovider::Constant(key, value) );
    }
    probeContextProviderForCid  = new wns::probe::bus::contextprovider::Variable("cid", 0);
    probeContextProviderForPriority  = new wns::probe::bus::contextprovider::Variable("MAC.QoSClass", 0);
    localContext.addProvider(wns::probe::bus::contextprovider::Container(probeContextProviderForCid));
    localContext.addProvider(wns::probe::bus::contextprovider::Container(probeContextProviderForPriority));
    std::string sizeProbeName = config.get<std::string>("sizeProbeName");
    sizeProbeBus = wns::probe::bus::ContextCollectorPtr(new wns::probe::bus::ContextCollector(localContext, sizeProbeName));
    std::string segmentHeaderCommandName = config.get<std::string>("segmentHeaderCommandName");
    segmentHeaderReader = myFUN->getCommandReader(segmentHeaderCommandName);
    assure(segmentHeaderReader, "No reader for the Segment Header ("<<segmentHeaderCommandName<<") available!");
    MESSAGE_SINGLE(NORMAL, logger, "SegmentingQueue::setFUN(): segmentHeaderCommandName="<<segmentHeaderCommandName);
}

bool SegmentingQueue::isAccepting(const wns::ldk::CompoundPtr&  compound ) const {
    int compoundSize = compound->getLengthInBits();
    ConnectionID cid = colleagues.registry->getCIDforPDU(compound);

    // if this is a brand new connection, return true because couldn't have
    // exceeded limit
    if (queues.find(cid) == queues.end())
    {
        MESSAGE_BEGIN(VERBOSE, logger, m, "");
        m << "Accepting PDU of size " <<  compoundSize <<  " into queue that would be newly created for CID=" << cid
          << " of user=" << colleagues.registry->getNameForUser(colleagues.registry->getUserForCID(cid)) <<"\n";
        MESSAGE_END();
        return true;
    }

    if (compoundSize + queues.find(cid)->second.bitsNetto > maxSize)
    {
        MESSAGE_BEGIN(VERBOSE, logger, m, "");
        m << "Not accepting PDU of size=" << compoundSize
          << " because net queuesize=" << queues.find(cid)->second.bitsNetto << " for CID=" << cid
          << " of user=" << colleagues.registry->getNameForUser(colleagues.registry->getUserForCID(cid)) <<"\n";
        MESSAGE_END();
        return  false;
    }

    MESSAGE_BEGIN(VERBOSE, logger, m, "");
    m << "Accepting PDU of size=" <<  compoundSize <<  " because net queuesize=" << queues.find(cid)->second.bitsNetto << " for CID=" << cid
      << " of user=" << colleagues.registry->getNameForUser(colleagues.registry->getUserForCID(cid)) <<"\n";
    MESSAGE_END();
    return true;
} // isAccepting

void
SegmentingQueue::put(const wns::ldk::CompoundPtr& compound) {
    assure(compound, "No valid PDU");
    assure(compound != wns::ldk::CompoundPtr(), "No valid PDU" );
    assure(isAccepting(compound),"sendData() has been called without isAccepting()");
    assure(colleagues.registry, "Need a registry as colleague, please set first");

    ConnectionID cid = colleagues.registry->getCIDforPDU(compound);
    Bit compoundLength = compound->getLengthInBits();
    assure(compoundLength>0,"compoundLength="<<compoundLength);

    // saves pdu and automatically create new queue if necessary
    // needs a 'map' to do so.
    queues[cid].pduQueue.push_back(compound);
    queues[cid].bitsNetto  += compoundLength;
    queues[cid].bitsBrutto += compoundLength*extensionHeaderSize;
    // ^ TODO: is this correct [->dbn]

    MESSAGE_SINGLE(NORMAL, logger, "SegmentingQueue::put(cid="<<cid<<"): after: bits="<<queues[cid].bitsNetto<<"/"<<queues[cid].bitsBrutto<<", PDUs="<<queues[cid].pduQueue.size());

    if (probeContextProviderForCid && probeContextProviderForPriority && sizeProbeBus) {
        probeContextProviderForCid->set(cid /*int context*/);
        int priority = colleagues.registry->getPriorityForConnection(cid); // only for probes
        probeContextProviderForPriority->set(priority);
        sizeProbeBus->put((double)queues[cid].bitsBrutto / (double)maxSize); // relative (0..100%)
    } else {
        MESSAGE_SINGLE(NORMAL, logger, "SegmentingQueue::put(cid="<<cid<<"): size="<<queues[cid].bitsBrutto<<"): undefined sizeProbeBus="<<sizeProbeBus);
    }
} // put

// [rs]: obsolete? Better use cid-related questions. Used frequently in OLD scheduler strategies
UserSet
SegmentingQueue::getQueuedUsers() const {
    UserSet users;

    for (QueueContainer::const_iterator iter = queues.begin(); iter != queues.end(); ++iter)
    {
        if ((*iter).second.pduQueue.size() != 0)
        {
            ConnectionID cid = iter->first;
            UserID user = colleagues.registry->getUserForCID(cid);
            users.insert(user);
        }
    }
    return users;
}

ConnectionSet
SegmentingQueue::getActiveConnections() const
{
    ConnectionSet result;

    for (QueueContainer::const_iterator iter = queues.begin(); iter != queues.end(); ++iter) {
        ConnectionID cid = iter->first;
        if ((*iter).second.pduQueue.size() != 0) {
            result.insert(cid);
        } else {
            assure(iter->second.bitsNetto==0,"Zero packets but "<<iter->second.bitsNetto<<" bits. How can this be?");
        }
    }
    return result;
}

uint32_t
SegmentingQueue::numCompoundsForCid(ConnectionID cid) const
{
    //return queues[cid].pduQueue.size();
    QueueContainer::const_iterator iter = queues.find(cid);
    assure(iter != queues.end(),"cannot find queue for cid="<<cid);
    return iter->second.pduQueue.size();
}

uint32_t
SegmentingQueue::numBitsForCid(ConnectionID cid) const
{
    //return queues[cid].bitsBrutto;
    QueueContainer::const_iterator iter = queues.find(cid);
    assure(iter != queues.end(),"cannot find queue for cid="<<cid);
    int numCompounds = iter->second.pduQueue.size();
    if (numCompounds==0) return 0;
    //assure(queueHasPDUs(cid), "getHeadOfLinePDUbits called for CID without PDUs or non-existent CID="<<cid);

    Bit remainingNettoBits = iter->second.bitsNetto; // - iter->second.frontSegmentSentBits;
    // if we have remainingBits we also must have a pdu:
    assure(remainingNettoBits>0,"remainingNettoBits="<<remainingNettoBits<<" but numCompounds="<<numCompounds);
    Bit numBits = remainingNettoBits + fixedHeaderSize + (numCompounds - 1) * extensionHeaderSize;
    // TODO: make this work:
    //assure(numBits == iter->second.bitsBrutto, "numBits="<<numBits<<" vs queues["<<cid<<"].bitsBrutto="<<iter->second.bitsBrutto);
    //numBits = queues[cid].bitsBrutto; // can be used if assure is always true
    return numBits;
} // numBitsForCid()

// result is sorted per-cid
QueueStatusContainer
SegmentingQueue::getQueueStatus() const
{
    wns::scheduler::QueueStatusContainer result;

    // Find all queues
    for (QueueContainer::const_iterator iter = queues.begin(); iter != queues.end(); ++iter)
    {
        ConnectionID cid = iter->first;
        QueueStatus queueStatus;
        queueStatus.numOfBits      = iter->second.bitsBrutto;
        queueStatus.numOfCompounds = iter->second.pduQueue.size();
        result.insert(cid,queueStatus);
        MESSAGE_SINGLE(NORMAL, logger, "SegmentingQueue::getQueueStatus():"
                       << " for cid=" << cid
                       << ": bits=" << queueStatus.numOfBits
                       << ", PDUs=" << queueStatus.numOfCompounds);
        // if we have bits we also must have a pdu:
        assure((queueStatus.numOfBits==0)
               || (queueStatus.numOfCompounds>0),
               "numOfBits="<<queueStatus.numOfBits<<" but numOfCompounds="<<queueStatus.numOfCompounds<<" for cid="<<cid);
    }
    return result;
} // getQueueStatus()

wns::ldk::CompoundPtr
SegmentingQueue::getHeadOfLinePDU(ConnectionID cid) {
    assure(false, "The SegmentingQueue does not support getHeadOfLinePDU");
}

// return only those bits which belong to one PDU
int
SegmentingQueue::getHeadOfLinePDUbits(ConnectionID cid)
{
    QueueContainer::const_iterator iter = queues.find(cid);
    assure(iter != queues.end(),"cannot find queue for cid="<<cid);
    assure(queueHasPDUs(cid), "getHeadOfLinePDUbits called for CID without PDUs or non-existent CID="<<cid);
    Bit remainingNettoBits = iter->second.pduQueue.front()->getLengthInBits() - iter->second.frontSegmentSentBits;
    Bit remainingBruttoBits = remainingNettoBits + fixedHeaderSize; // one PDU
    // TODO: make this work:
    //assure(remainingNettoBits>0,"remainingNettoBits="<<remainingNettoBits<<" but PDUs="<<iter->second.pduQueue.size());
    //assure(remainingNettoBits>0,"remainingNettoBits="<<remainingNettoBits<<" but PDUs="<<iter->second.pduQueue.size()
    //       <<", r=("<<iter->second.pduQueue.front()->getLengthInBits()<<"-"<<iter->second.frontSegmentSentBits<<")"
    //       <<", numBitsForCid="<<numBitsForCid(cid));
    remainingBruttoBits = numBitsForCid(cid); // return complete #bits of _ALL_ PDUs.
    return remainingBruttoBits;
}

wns::ldk::CompoundPtr
SegmentingQueue::getHeadOfLinePDUSegment(ConnectionID cid, int requestedBits)
{
    assure(queueHasPDUs(cid), "getHeadOfLinePDUSegments(cid="<<cid<<",bits="<<requestedBits<<") called for CID without PDUs or non-existent CID");
    assure(requestedBits>fixedHeaderSize, "The segment size requestedBits=" << requestedBits << " must be larger than the fixedHeaderSize=" << fixedHeaderSize);

    wns::ldk::CompoundPtr pdu(queues[cid].pduQueue.front()->copy());

    segmentHeaderReader->activateCommand(pdu->getCommandPool());

    ISegmentationCommand* header = segmentHeaderReader->readCommand<ISegmentationCommand>(pdu->getCommandPool());

    header->increaseHeaderSize(fixedHeaderSize);

    header->setSequenceNumber(queues[cid].currentSegmentNumber);
    queues[cid].currentSegmentNumber += 1;

    (queues[cid].frontSegmentSentBits == 0) ? header->setBeginFlag():header->clearBeginFlag();

    while (header->totalSize() < requestedBits)
    {
        wns::ldk::CompoundPtr c = queues[cid].pduQueue.front();
        Bit length = c->getLengthInBits() - queues[cid].frontSegmentSentBits; // netto
        Bit capacity = requestedBits - header->totalSize(); // netto
        if (capacity >= length)
        { // fits in completely
            header->addSDU(c->copy());
            header->increaseDataSize(length);
            queues[cid].pduQueue.pop_front();
            queues[cid].frontSegmentSentBits = 0;
            queues[cid].bitsNetto -= length;

            if ( (header->totalSize() + extensionHeaderSize < requestedBits) &&
                 (queues[cid].pduQueue.size() > 0))
            {
                header->increaseHeaderSize(extensionHeaderSize);
            }
            else
            {
                break;
            }
        }
        else
        { // only a fraction fits in
            header->addSDU(c->copy());
            header->increaseDataSize(capacity);
            queues[cid].frontSegmentSentBits += capacity;
            queues[cid].bitsNetto -= capacity;
            assure(queues[cid].frontSegmentSentBits < queues[cid].pduQueue.front()->getLengthInBits(), "frontSegmentSentBits is larger than the front PDU size!");
        }
    }

    // remaining bits:
    int numCompounds = queues[cid].pduQueue.size();
    if (numCompounds==0)
    {
        queues[cid].bitsNetto  = 0;
        queues[cid].bitsBrutto = 0;
    } else {
        //queues[cid].bitsNetto should be correct
        Bit remainingNettoBits = queues[cid].bitsNetto;
        // TODO: fix this workaround:
        if ((remainingNettoBits==0) && (numCompounds>0)) {
            MESSAGE_SINGLE(NORMAL, logger, "ERROR: getHeadOfLinePDUSegment(cid="<<cid<<",to="<<colleagues.registry->getNameForUser(colleagues.registry->getUserForCID(cid))
                           <<",bits="<<requestedBits<<"): numCompounds="<<numCompounds<<" but remainingNettoBits="<<remainingNettoBits);
            queues[cid].pduQueue.pop_front();
            numCompounds--;
            queues[cid].bitsBrutto = 0; // because remainingNettoBits==0
        } else {
            queues[cid].bitsBrutto = remainingNettoBits + fixedHeaderSize + (numCompounds - 1) * extensionHeaderSize;
        }
    }

    if (probeContextProviderForCid && probeContextProviderForPriority && sizeProbeBus) {
        probeContextProviderForCid->set(cid /*int context*/);
        int priority = colleagues.registry->getPriorityForConnection(cid);
        probeContextProviderForPriority->set(priority);
        //sizeProbeBus->put(queues[cid].bits /*double wert*/); // absolute bits
        sizeProbeBus->put((double)queues[cid].bitsBrutto / (double)maxSize); // relative (0..100%)
    }

    (queues[cid].frontSegmentSentBits == 0) ? header->setEndFlag():header->clearEndFlag();

    // Rest is padding (optional)
    if (usePadding) {
        header->increasePaddingSize(requestedBits - header->totalSize());
    }
    // TODO !!! see SAR.hpp
    // this->commitSizes(pdu->getCommandPool());
    // commandPoolSize = this->getCommandSize(); // look for this in SAR.hpp

    MESSAGE_SINGLE(NORMAL, logger, "getHeadOfLinePDUSegment(cid="<<cid<<",to="<<colleagues.registry->getNameForUser(colleagues.registry->getUserForCID(cid))
                   <<",bits="<<requestedBits<<"): totalSize="<<header->totalSize()<<" bits, sn="<<queues[cid].currentSegmentNumber-1);
    assure(header->totalSize()<=requestedBits,"pdulength="<<header->totalSize()<<" > bits="<<requestedBits);
    return pdu;
}

bool
SegmentingQueue::isEmpty() const
{
    for (QueueContainer::const_iterator iter = queues.begin(); iter != queues.end(); ++iter)
    {
        if ((*iter).second.pduQueue.size() != 0)
            return false;
    }
    return true;
}

bool
SegmentingQueue::hasQueue(ConnectionID cid)
{
    return queues.find(cid) != queues.end();
}

bool
SegmentingQueue::queueHasPDUs(ConnectionID cid) {
    if (queues.find(cid) == queues.end())
        return false;
    return (queues[cid].pduQueue.size() != 0);
}

ConnectionSet
SegmentingQueue::filterQueuedCids(ConnectionSet connections) {
    ConnectionSet activeConnections;
    for ( wns::scheduler::ConnectionSet::iterator iter = connections.begin(); iter != connections.end(); ++iter )
    {
        ConnectionID cid = *iter;
        if ( queueHasPDUs(cid) )
            activeConnections.insert(cid);
    }
    return activeConnections;
}

void
SegmentingQueue::setColleagues(RegistryProxyInterface* _registry) {
    colleagues.registry = _registry;
    maxSize = colleagues.registry->getQueueSizeLimitPerConnection();
}

QueueInterface::ProbeOutput
SegmentingQueue::resetAllQueues()
{
    // Store number of bits and compounds for Probe which will be deleted
    ProbeOutput probeOutput;
    for (QueueContainer::iterator iter = queues.begin();
         iter != queues.end(); ++iter)
    {
        ConnectionID cid = iter->first;
        probeOutput.bits += iter->second.bitsBrutto;
        probeOutput.compounds += iter->second.pduQueue.size();
        if (probeContextProviderForCid && probeContextProviderForPriority && sizeProbeBus) {
            probeContextProviderForCid->set(cid);
            int priority = colleagues.registry->getPriorityForConnection(cid);
            probeContextProviderForCid->set(priority);
            sizeProbeBus->put(0.0 /*double wert*/);
        }
    }

    // queues is a std::map that stores the std::queues that store the
    // CompoundPtrs. So by doing a queues.clear(), the destructors are called
    // and the refCounting mechanism of the CompoundPtr takes care of actually
    // deleting the compounds.
    queues.clear();

    return probeOutput;
}

// [rs]: obsolete? Better use cid-related questions
QueueInterface::ProbeOutput
SegmentingQueue::resetQueues(UserID _user)
{
    // MESSAGE_SINGLE(NORMAL, logger, "SegmentingQueue::resetQueues(): obsolete"); // TODO [rs]; not supported by [aoz]
    // Store number of bits and compounds for Probe which will be deleted
    ProbeOutput probeOutput;

    // Find all queues that belong to this user and delete them.  This one is a
    // little bit tricky, see section 6.6.2 of Josutti's STL book: we have to be
    // careful when deleting the current iterator position
    for (QueueContainer::iterator iter = queues.begin(); iter != queues.end(); )
    {
        ConnectionID cid = iter->first;
        UserID user = colleagues.registry->getUserForCID(cid);
        if (user == _user)
        {
            ConnectionID cid = iter->first;
            probeOutput.bits += iter->second.bitsBrutto;
            probeOutput.compounds += iter->second.pduQueue.size();
            if (probeContextProviderForCid && probeContextProviderForPriority && sizeProbeBus) {
                probeContextProviderForCid->set(cid);
                int priority = colleagues.registry->getPriorityForConnection(cid);
                probeContextProviderForCid->set(priority);
                sizeProbeBus->put(0.0 /*double wert*/);
            }
            queues.erase(iter++);
        }
        else
            ++iter;
    }
    return probeOutput;
}

QueueInterface::ProbeOutput
SegmentingQueue::resetQueue(ConnectionID cid)
{
    // Store number of bits and compounds for Probe which will be deleted
    ProbeOutput probeOutput;
    probeOutput.bits += queues[cid].bitsBrutto;
    probeOutput.compounds += queues[cid].pduQueue.size();
    if (probeContextProviderForCid && probeContextProviderForPriority && sizeProbeBus) {
        probeContextProviderForCid->set(cid /*int context*/);
        int priority = colleagues.registry->getPriorityForConnection(cid);
        probeContextProviderForCid->set(priority);
        sizeProbeBus->put(0.0 /*double wert*/);
    }

#ifndef NDEBUG
    int numRemoved =
#endif
        queues.erase(cid);
    assure(numRemoved == 1, "Non-existing or too many queues with that CID");

    return probeOutput;
}

std::string
SegmentingQueue::printAllQueues()
{
    std::stringstream s;
    for (QueueContainer::iterator iter = queues.begin();
         iter != queues.end(); ++iter)
    {
        ConnectionID cid = iter->first;
        int bits      = iter->second.bitsBrutto;
        int compounds = iter->second.pduQueue.size();
        s << cid << ":" << bits << "," << compounds << " ";
    }
    return s.str();
}


