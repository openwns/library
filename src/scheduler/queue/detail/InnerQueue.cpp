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

#include <WNS/scheduler/queue/detail/InnerQueue.hpp>

#include <WNS/scheduler/queue/ISegmentationCommand.hpp>
#include <WNS/ldk/CommandTypeSpecifier.hpp>

using namespace wns::scheduler::queue::detail;

InnerQueue::InnerQueue():
    nettoBits_(0),
    sequenceNumber_(0),
    frontSegmentSentBits_(0)
{
}

Bit
InnerQueue::queuedNettoBits() const
{
    return nettoBits_;
}

Bit
InnerQueue::queuedBruttoBits(Bit fixedHeaderSize, Bit extensionHeaderSize) const
{
    if (queuedNettoBits() == 0)
    {
        return 0;
    }
    else
    {
        return fixedHeaderSize + (pduQueue_.size() - 1) * extensionHeaderSize + queuedNettoBits();
    }
}

int
InnerQueue::queuedCompounds() const
{
    return pduQueue_.size();
}

bool
InnerQueue::empty() const
{
    return pduQueue_.empty();
}

void
InnerQueue::put(const wns::ldk::CompoundPtr& compound)
{
    pduQueue_.push_back(compound);

    nettoBits_ += compound->getLengthInBits();
}

wns::ldk::CompoundPtr
InnerQueue::retrieve(Bit requestedBits, Bit fixedHeaderSize, Bit extensionHeaderSize, bool usePadding, wns::ldk::CommandReaderInterface* reader)
{
    if (requestedBits <= fixedHeaderSize)
    {
        throw RequestBelowMinimumSize(requestedBits, fixedHeaderSize);
    }

    if (pduQueue_.size() <= 0)
    {
        throw RetrieveException("Queue is empty");
    }

    wns::ldk::CompoundPtr pdu(pduQueue_.front()->copy());

    assure(reader != NULL, "No valid segmentHeaderReader given!");

    reader->activateCommand(pdu->getCommandPool());

    ISegmentationCommand* header = reader->readCommand<ISegmentationCommand>(pdu->getCommandPool());

    header->increaseHeaderSize(fixedHeaderSize);

    header->setSequenceNumber(sequenceNumber_);
    sequenceNumber_ += 1;

    (frontSegmentSentBits_ == 0) ? header->setBeginFlag():header->clearBeginFlag();

    while (header->totalSize() < requestedBits)
    {
        wns::ldk::CompoundPtr c = pduQueue_.front();
        Bit length = c->getLengthInBits() - frontSegmentSentBits_; // netto
        Bit capacity = requestedBits - header->totalSize(); // netto
        if (capacity >= length)
        {
            // fits in completely
            header->addSDU(c->copy());
            header->increaseDataSize(length);
            pduQueue_.pop_front();
            frontSegmentSentBits_ = 0;
            nettoBits_ -= length;

            if ( (header->totalSize() + extensionHeaderSize < requestedBits) &&
                 (pduQueue_.size() > 0))
            {
                header->increaseHeaderSize(extensionHeaderSize);
            }
            else
            {
                break;
            }
        }
        else
        {
            // only a fraction fits in
            header->addSDU(c->copy());
            header->increaseDataSize(capacity);
            frontSegmentSentBits_ += capacity;
            nettoBits_ -= capacity;
            assure(frontSegmentSentBits_ < pduQueue_.front()->getLengthInBits(), "frontSegmentSentBits_ is larger than the front PDU size!");
        }
    }

    // remaining bits:
    if (pduQueue_.size()==0)
    {
        nettoBits_ = 0;
    }

    // Is the last segment in the PDU a fragment? Then clear end flag
    (frontSegmentSentBits_ == 0) ? header->setEndFlag():header->clearEndFlag();

    // Rest is padding (optional)
    if (usePadding) {
        header->increasePaddingSize(requestedBits - header->totalSize());
    }

    assure(header->totalSize()<=requestedBits,"pdulength="<<header->totalSize()<<" > bits="<<requestedBits);
    return pdu;
}
