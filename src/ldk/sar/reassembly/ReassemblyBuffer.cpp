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

#include <WNS/ldk/sar/reassembly/ReassemblyBuffer.hpp>
#include <WNS/ldk/sar/SegAndConcat.hpp>

#include <sstream>

using namespace wns::ldk::sar::reassembly;

ReassemblyBuffer::ReassemblyBuffer():
    commandReader_(NULL)
{
}

void
ReassemblyBuffer::initialize(wns::ldk::CommandReaderInterface* cr)
{
    assure(cr != NULL, "Invalid command reader given in ReassemblyBuffer");

    commandReader_ = cr;

    assure(integrityCheck(), "Invalid state after initialization in ReassemblyBuffer");
}

bool
ReassemblyBuffer::isEmpty()
{
    return buffer_.size() == 0;
}

long
ReassemblyBuffer::getNextExpectedSN()
{
    if (isEmpty())
    {
        return -1;
    }
    else
    {
        return (1 + readCommand(buffer_.back())->peer.sn_);
    }
}

bool
ReassemblyBuffer::isNextExpectedSegment(const wns::ldk::CompoundPtr& c)
{
    if (isEmpty() && readCommand(c)->getBeginFlag())
    {
        return true;
    }

    long expectedSN = getNextExpectedSN();
    long currentSN = readCommand(c)->peer.sn_;

    if (expectedSN == currentSN)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool
ReassemblyBuffer::accepts(const wns::ldk::CompoundPtr& c)
{
    SegAndConcatCommand* command = NULL;
    command = readCommand(c);

    if (isEmpty() && command->getBeginFlag())
    {
        return true;
    }
    else
    {
        return isNextExpectedSegment(c);
    }
}

void
ReassemblyBuffer::insert(wns::ldk::CompoundPtr c)
{
    assure(accepts(c), "Trying to insert an unacceptable compound");
    buffer_.push_back(c);

    integrityCheck();
}

wns::ldk::sar::SegAndConcatCommand*
ReassemblyBuffer::readCommand(const wns::ldk::CompoundPtr& c)
{
    wns::ldk::sar::SegAndConcatCommand* command = NULL;
    command = commandReader_->readCommand<SegAndConcatCommand>(c->getCommandPool());
    assure(command!=NULL, "Command could not be retrieved");
    return command;
}

ReassemblyBuffer::SegmentContainer
ReassemblyBuffer::getReassembledSegments(int &reassembledSegmentCounter)
{
    reassembledSegmentCounter = 0;

    assure(integrityCheck(), "IntegrityCheck failed!");

    ReassemblyBuffer::SegmentContainer sc;

    while (buffer_.size() > 0)
    {
        assure(integrityCheck(), "IntegrityCheck failed!");

        SegAndConcatCommand* frontCommand = readCommand(buffer_.front());

        if (frontCommand->getEndFlag())
        {
            // PDU starts with an SDU and ends with an SDU, we can reassemble
            // all SDUs within this segment
            std::list<wns::ldk::CompoundPtr>::iterator it;

            for(it = frontCommand->peer.pdus_.begin(); it!=frontCommand->peer.pdus_.end(); ++it)
            {
                sc.push_back( (*it) );
            }

            buffer_.pop_front();
            reassembledSegmentCounter++;

            // Whole SDU was unpacked. IntegrityCheck must hold.
            assure(integrityCheck(), "IntegrityCheck failed!");
        }
        else
        {
            // Is there a follow-up PDU with either more than one SDU
            // or with the end flag set? If yes, we can reassemble
            ReassemblyBuffer::SegmentContainer::iterator endOfSDU;

            for(endOfSDU = buffer_.begin();
                endOfSDU!=buffer_.end();
                ++endOfSDU)
            {
                SegAndConcatCommand* currentCommand = readCommand(*endOfSDU);

                if (currentCommand->getEndFlag() || (currentCommand->peer.pdus_.size() > 1))
                {
                    break;
                }
            }

            // If we can reassemble
            if (endOfSDU != buffer_.end())
            {
                while(buffer_.size() > 0)
                {
                    SegAndConcatCommand* frontCommand = readCommand(buffer_.front());

                    if (frontCommand->peer.pdus_.size() > 1)
                    {
                        sc.push_front(frontCommand->peer.pdus_.front());
                        frontCommand->peer.pdus_.pop_front();
                        reassembledSegmentCounter++;
                        frontCommand->setBeginFlag();
                        break;
                    }
                    else
                    {
                        sc.push_back(frontCommand->peer.pdus_.front());
                    }

                    if (frontCommand->getEndFlag())
                    {
                        buffer_.pop_front();
                        reassembledSegmentCounter++;
                        break;
                    }
                    else
                    {
                        buffer_.pop_front();
                        reassembledSegmentCounter++;
                        // The next segment continues this SDU
                        // It needs to be dropped, since we already pushed the
                        // first segment to the segment container
                        reassembledSegmentCounter += dropSegmentsOfSDU();
                        break;
                    }
                }
            }
            else
            {
                break;
            }
        }
    }

    return sc;
} // getReassembledSegments

std::string
ReassemblyBuffer::dump()
{
    std::stringstream s;
    SegmentContainer::iterator it;

    for(it=buffer_.begin(); it!=buffer_.end(); ++it)
    {
        SegAndConcatCommand* c = readCommand(*it);

        s << "|(" << std::string(c->getBeginFlag() ? "B":"")
          << std::string(c->getEndFlag() ? "E":"")
          << ")|" << c->peer.pdus_.size() << ":(";

        std::list<wns::ldk::CompoundPtr>::iterator pit;

        for(pit=c->peer.pdus_.begin(); pit!=c->peer.pdus_.end(); ++pit)
        {
            s << (*pit)->getBirthmark() << ",";
        }
        s << ")   ";
    }

    return s.str();
}

void
ReassemblyBuffer::clear()
{
    buffer_.clear();
}

size_t
ReassemblyBuffer::size()
{
    return buffer_.size();
}

bool
ReassemblyBuffer::integrityCheck()
{
    if (commandReader_ == NULL)
        return false;

    if (buffer_.size() == 0)
    {
        return true;
    }

    if (!(readCommand(buffer_.front())->peer.isBegin_))
    {
        return false;
    }

    return true;
}

int
ReassemblyBuffer::dropSegmentsOfSDU()
{
    assure(buffer_.size() > 0, "You cannot drop SDU segments from an empty buffer!");
    // This first Segment in buffer_ is the first to be dropped
    // until the next segment border
    int droppedSegmentCounter = 0;

    while(true)
    {
        SegAndConcatCommand* frontCommand = readCommand(buffer_.front());
        if ( (frontCommand->peer.pdus_.size() == 1) && (frontCommand->getEndFlag()) )
        {
            buffer_.pop_front();
            droppedSegmentCounter++;
            break;
        }

        if (frontCommand->peer.pdus_.size() > 1)
        {
            frontCommand->peer.pdus_.pop_front();
            frontCommand->setBeginFlag();
            break;
        }

        if ( (frontCommand->peer.pdus_.size() == 1) && (!frontCommand->getEndFlag()) )
        {
            buffer_.pop_front();
            droppedSegmentCounter++;
            continue;
        }
    }
    return droppedSegmentCounter;
}
