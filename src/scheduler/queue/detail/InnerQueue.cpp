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

#include <WNS/simulator/ISimulator.hpp>
#include <WNS/ldk/probe/TickTack.hpp>


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
InnerQueue::queuedBruttoBits(Bit fixedHeaderSize, Bit extensionHeaderSize, bool byteAlignHeader, Bit maxCB, Bit crc) const
{
    if (queuedNettoBits() == 0)
    {
        return 0;
    }
    else
    {
        Bit headerSize = fixedHeaderSize + (pduQueue_.size() - 1) * extensionHeaderSize;
        if (byteAlignHeader)
        {
            headerSize += headerSize % 8;
        }

        if(crc > 0)
        {        
            int numCBs = ceil(double(headerSize + queuedNettoBits()) / double(maxCB));
            return headerSize + queuedNettoBits() + numCBs * crc;
        }
        else
            return headerSize + queuedNettoBits();
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
InnerQueue::retrieve(Bit requestedBits, Bit fixedHeaderSize, Bit extensionHeaderSize, 
    bool usePadding, bool byteAlignHeader,
    wns::ldk::CommandReaderInterface* reader,
    const wns::probe::bus::ContextCollectorPtr& probeCC,
    wns::ldk::CommandReaderInterface* probeCmdReader)
{
    return retrieve(requestedBits, fixedHeaderSize, extensionHeaderSize,
                    usePadding, byteAlignHeader, 1E6, 0, reader, probeCC,
                    probeCmdReader);
}

wns::ldk::CompoundPtr
InnerQueue::retrieve(Bit requestedBits, Bit fixedHeaderSize, Bit extensionHeaderSize, 
    bool usePadding, bool byteAlignHeader,
    Bit maxCB, Bit crc, 
    wns::ldk::CommandReaderInterface* reader,
    const wns::probe::bus::ContextCollectorPtr& probeCC,
    wns::ldk::CommandReaderInterface* probeCmdReader)
{
    if (requestedBits <= fixedHeaderSize)
    {
        throw RequestBelowMinimumSize(requestedBits, fixedHeaderSize);
    }

    if (pduQueue_.size() <= 0)
    {
        throw RetrieveException("Queue is empty");
    }

    int fs = frontSegmentSentBits_;


    wns::ldk::CompoundPtr pdu(pduQueue_.front()->copy());

    assure(reader != NULL, "No valid segmentHeaderReader given!");

    reader->activateCommand(pdu->getCommandPool());

    ISegmentationCommand* header = reader->readCommand<ISegmentationCommand>(pdu->getCommandPool());

    header->setSequenceNumber(sequenceNumber_);
    sequenceNumber_ += 1;

    (frontSegmentSentBits_ == 0) ? header->setBeginFlag():header->clearBeginFlag();;

    CompoundContainer::iterator it;

    Bit totalPayloadSize = 0;
    Bit totalPDUSize = 0;
    Bit totalHeaderSize = 0;
    Bit crcHeaderSize = 0;
    Bit futureHeaderSize = 0;
    Bit futureExtensionHeaderSize = 0;
    Bit futurePDUSizeNoCRC = 0;
    Bit futureCRCSize = 0;
    Bit futureAlignment = 0;
    unsigned int numSegments = 0;

    for(it = pduQueue_.begin(); it != pduQueue_.end(); it++)
    {
        numSegments++;
        totalPayloadSize += (*it)->getLengthInBits();
        if(it == pduQueue_.begin())
            totalPayloadSize -= frontSegmentSentBits_;
        futureExtensionHeaderSize = (numSegments - 1) * extensionHeaderSize;

        if(byteAlignHeader)
        {
            futureAlignment = futureExtensionHeaderSize % 8;
            futureExtensionHeaderSize += futureAlignment;
        }
     
        futureHeaderSize = fixedHeaderSize + futureExtensionHeaderSize;
        futurePDUSizeNoCRC = totalPayloadSize + futureHeaderSize;
        futureCRCSize = ceil(double(futurePDUSizeNoCRC) / double(maxCB)) * crc;
        totalHeaderSize = futureHeaderSize + futureCRCSize;

        if(futureCRCSize + futurePDUSizeNoCRC >= requestedBits)
            break;
    }
    // Emptied the entire Queue:
    if(it == pduQueue_.end())
    {
        header->setEndFlag();
        for(it = pduQueue_.begin(); it != pduQueue_.end(); it++)
        {
            Bit length = (*it)->getLengthInBits() - frontSegmentSentBits_;
            header->addSDU((*it)->copy());
            header->increaseDataSize(length);
            probe(*it, probeCC, probeCmdReader); 
            frontSegmentSentBits_ = 0;
            nettoBits_ -= length;
        }
        for(int n = numSegments; n > 0; n--)
        {
            pduQueue_.pop_front();
        }

        header->increaseHeaderSize(totalHeaderSize);
        assure(totalPayloadSize == header->dataSize(), "Size mismatch after queue emtied.");
        assure(nettoBits_ == 0, "Queue not emptied.");
    }
    else
    {
        // Exact fit (very unlikely)
        if(futureCRCSize + futurePDUSizeNoCRC == requestedBits)
        {
            header->setEndFlag();
            int i = 0;
            for(it = pduQueue_.begin(); i < numSegments; it++)
            {
                Bit length = (*it)->getLengthInBits() - frontSegmentSentBits_;
                header->addSDU((*it)->copy());
                header->increaseDataSize(length);
                probe(*it, probeCC, probeCmdReader); 
                frontSegmentSentBits_ = 0;
                nettoBits_ -= length;
                i++;
            }
            for(int n = i; n > 0; n--)
            {
                pduQueue_.pop_front();
            }

            header->increaseHeaderSize(totalHeaderSize);
            assure(requestedBits == header->totalSize(), "Size mismatch after exact hit.");
        }
        // Last segment is part of SDU (most likely case)
        else
        {
            int i = 0;
            for(it = pduQueue_.begin(); i < numSegments - 1; it++)
            {
                Bit length = (*it)->getLengthInBits() - frontSegmentSentBits_;
                header->addSDU((*it)->copy());
                header->increaseDataSize(length);
                probe(*it, probeCC, probeCmdReader); 
                frontSegmentSentBits_ = 0;
                nettoBits_ -= length;            
                i++;
            }
            assure(it != pduQueue_.end(), "No segment left for partial retrieve.");
            for(int n = i; n > 0; n--)
            {
                pduQueue_.pop_front();
            }

            // Do not include CRC header yet and exclude extHeader for last segment 
            // and possible byte alignment padding bit
            // Could be that only parts of one SDU fit, then there is no extHeader
            Bit newHeader = std::max(fixedHeaderSize, futureHeaderSize - extensionHeaderSize - futureAlignment);
            header->increaseHeaderSize(newHeader);

            Bit space = requestedBits - header->totalSize() - futureCRCSize;

            assure(header->totalSize() <= requestedBits, "Not all segments processed but already full.");

            // Could be we just exactly fit the extension header or parts of it 
            // but no more segments. It does not make sense then to include another segment
            // Could also be we only fit parts of one SDU, in this case there is no extension
            // header.
            Bit nextHeader;
            if(numSegments > 1)
                nextHeader = extensionHeaderSize;
            else
                nextHeader = 0;

            if(byteAlignHeader)
                nextHeader += (header->totalSize() + nextHeader) % 8;

            if(space > nextHeader)
            {
                header->increaseHeaderSize(nextHeader);

                // Could be that we need less Code Blocks (CBs) for the new total PDU size
                futurePDUSizeNoCRC = header->totalSize() + space;
                futureCRCSize = ceil(double(futurePDUSizeNoCRC) / double(maxCB)) * crc;
                space = requestedBits - header->totalSize() - futureCRCSize;            

                frontSegmentSentBits_ += space;
                (frontSegmentSentBits_ == 0) ? header->setEndFlag():header->clearEndFlag();

                header->addSDU(pduQueue_.front()->copy());
                header->increaseDataSize(space);
                header->increaseHeaderSize(futureCRCSize);
                nettoBits_ -= space;
            }
            else
            {               
                header->increaseHeaderSize(
                    ceil(double(header->totalSize()) / double(maxCB)) * crc);

                // Byte align the header
                if(byteAlignHeader)
                {
                    Bit align = header->headerSize() % 8;
                    header->increaseHeaderSize(align);
                }
            }
        }
        
        
    }

    // remaining bits:
    if (pduQueue_.size()==0)
    {
        nettoBits_ = 0;
    }

    // Rest is padding (optional)
    if (usePadding) {
        header->increasePaddingSize(requestedBits - header->totalSize());
    }

    assure(header->totalSize()<=requestedBits,"pdulength="<<header->totalSize()<<" > bits="<<requestedBits);

#ifndef WNS_NDEGUG
    Bit extHeader = (header->getNumSDUs() - 1) * extensionHeaderSize;
    if(byteAlignHeader)
        extHeader+= extHeader % 8;

    Bit beforeCRC = header->dataSize() + extHeader + fixedHeaderSize;
    Bit crcHeader = ceil(double(beforeCRC) / double(maxCB)) * crc;


    Bit totHeader = crcHeader + fixedHeaderSize + extHeader;
    assure(header->headerSize() == totHeader, "Header size mismatch: "
        << "\nFixed: " << fixedHeaderSize
        << "\nExt: " << extHeader
        << "\nCRC: " << crcHeader
        << "\nSDUs: " << header->getNumSDUs()
        << "\nPayload: " << header->dataSize()
        << "\nTotal header size: " << header->headerSize()
        << " != " << totHeader);
#endif

    return pdu;

}


std::queue<wns::ldk::CompoundPtr> 
InnerQueue::getQueueCopy()
{
    std::queue<wns::ldk::CompoundPtr> q;
    
    CompoundContainer::iterator it;
    
    for(it = pduQueue_.begin(); it != pduQueue_.end(); it++)
        q.push(*it);

    return q;
}

void
InnerQueue::probe(const wns::ldk::CompoundPtr& compound,
    const wns::probe::bus::ContextCollectorPtr& probeCC,
    wns::ldk::CommandReaderInterface* cmdReader)
{
    assure(compound != wns::ldk::CompoundPtr(), "No valid compound");

    if(cmdReader == NULL ||
        probeCC == wns::probe::bus::ContextCollectorPtr())
        return;

    wns::ldk::probe::TickTackCommand* cmd;
    cmd = cmdReader->readCommand<wns::ldk::probe::TickTackCommand>(compound->getCommandPool());
    assure(cmd != NULL, "Cannot get TickTackCommand to probe delay");

    wns::simulator::Time now = wns::simulator::getEventScheduler()->getTime();
    probeCC->put(compound, now - cmd->magic.tickTime);
}


