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

#include <WNS/scheduler/strategy/staticpriority/persistentvoip/ResourceGrid.hpp>

using namespace std;
using namespace wns::scheduler;
using namespace wns::scheduler::strategy;
using namespace wns::scheduler::strategy::staticpriority;
using namespace wns::scheduler::strategy::staticpriority::persistentvoip;



ResourceBlock::ResourceBlock(Frame* parent, unsigned int index) :
    parent_(parent),
    subChannel_(index),
    free_(true),
    neverUsed_(true),
    logger_(parent_->getLogger())
{
};

ResourceBlock::~ResourceBlock()
{
};

bool
ResourceBlock::operator<(const ResourceBlock& other) const
{
    return this->subChannel_ < other.subChannel_;
};  

bool
ResourceBlock::isFree() const
{
    return free_;
};  

void
ResourceBlock::setFree() 
{
    assure(!isFree(), "RB is already free.");
    free_ = true;
};  

void
ResourceBlock::setOccupied() 
{
    assure(isFree(), "RB is already occupied.");

    free_ = false;
};  

unsigned int
ResourceBlock::getSubChannelIndex()
{
    return subChannel_;
}

unsigned int
ResourceBlock::getFrameIndex()
{
    assure(parent_ != NULL, "Invalid frame pointer");

    return parent_->getFrameIndex();
}

TransmissionBlock::TransmissionBlock(ResourceBlockVectorIt& start, 
    ResourceBlockVectorIt& end,
    ConnectionID cid) :
    cid_(cid),
    length_(0)
{
    frame_ = start->getFrameIndex();

    ResourceBlockVectorIt it;
    for(it = start; it != end; it++)
    {
        assure(it->isFree(), "RB occupied");
        assure(it->getFrameIndex() == frame_, "RBs belong to different frames.");
        it->setOccupied();
        rbs_.insert(&(*it));
        length_++;
    }
    start_ = start->getSubChannelIndex();
} 

TransmissionBlock::~TransmissionBlock()
{
    ResourceBlockPointerSetIt it;
    for(it = rbs_.begin(); it != rbs_.end(); it++)
    {
        (*it)->setFree();
    }   
}

bool
TransmissionBlock::operator==(const TransmissionBlock& other) const
{
    return start_ == other.start_ && 
        length_ == other.length_ && 
        cid_ == other.cid_;    
}

bool
TransmissionBlock::operator<(const TransmissionBlock& other) const
{
    assure(start_ != other.start_, "Overlapping TBs");
    return start_ < other.start_;   
}

unsigned int
TransmissionBlock::getCID()
{
    return cid_;
}

Frame::Frame(ResourceGrid* parent,
        unsigned int index):
    parent_(parent),
    frame_(index),
    numberOfSubChannels_(parent->getSubChannelsPerFrame()),
    logger_(parent->getLogger())
{
    assure(numberOfSubChannels_ > 0, "Need more than zero subchannels");

    for(int i = 0; i < numberOfSubChannels_; i++)
    {
        rbs_.push_back(ResourceBlock(this, i));
    }
}

Frame::~Frame()
{
    unpersistentSchedule_.clear();
    persistentSchedule_.clear();
}

unsigned int
Frame::getFrameIndex()
{
    return frame_;
}

Frame::SearchResultSet
Frame::findTransmissionBlocks(unsigned int minLength)
{
    MESSAGE_SINGLE(NORMAL, *logger_, "Reserved: " << *this);

    unsigned int start = 0;

    SearchResult sr;
    SearchResultSet srs;

    do
    {
        sr = SearchResult();
        sr = findTransmissionBlock(start, minLength);
        if(sr.success)
        {
            srs.insert(sr);
            start = sr.start + sr.length;
        }
    }
    while(sr.success && start < numberOfSubChannels_);
    
    return srs;
}

Frame::SearchResult
Frame::findTransmissionBlock(unsigned int start, unsigned int minLength)
{
    SearchResult sr;

    for(int i = start; i < numberOfSubChannels_; i++)
    {
        if(rbs_[i].isFree())
        {
            int start = i;
            int nFree = 0;
            while(rbs_[i].isFree() && i < numberOfSubChannels_)
            {
                nFree++;
                i++;
            }
            if(nFree >= minLength)
            {
                sr.success = true;
                sr.start = start;
                sr.length = nFree;
                break;
            }
        }
    }
    return sr;    
}

void
Frame::reserve(ConnectionID cid, unsigned int st, unsigned int l, bool persistent)
{
    /* Further assures in TransmissionBlock will check the rest */
    assure(st < numberOfSubChannels_, "Request start exceeds number of subchannels");
    assure(st + l < numberOfSubChannels_, "Request exceeds number of subchannels");

    assure(persistentSchedule_.find(cid) == persistentSchedule_.end(), 
        "CID already scheduled persistently");

    assure(unpersistentSchedule_.find(cid) == unpersistentSchedule_.end(), 
        "CID already scheduled unpersistently");

    ResourceBlockVectorIt start;
    ResourceBlockVectorIt end;

    start = rbs_.begin() + st;
    end = start + l;

    TransmissionBlockPtr tb = TransmissionBlockPtr(new TransmissionBlock(start, end, cid));

    if(persistent)
        persistentSchedule_.insert(std::pair<ConnectionID, TransmissionBlockPtr>(cid, tb));
    else    
        unpersistentSchedule_.insert(std::pair<ConnectionID, TransmissionBlockPtr>(cid, tb));

    MESSAGE_SINGLE(NORMAL, *logger_, "Reserved " << l << " RBs strating at " 
        << st << " for CID " << cid << "; persistent: " << (persistent?"yes":"no"));
}

void
Frame::clearUnpersistentSchedule()
{
    MESSAGE_SINGLE(NORMAL, *logger_, "ClearUnpersistenSchedule frame " << frame_ 
        << " before: " << *this);

    unpersistentSchedule_.clear();    

    MESSAGE_SINGLE(NORMAL, *logger_, "ClearUnpersistenSchedule frame " << frame_ 
        << " after:  " << *this);
}

wns::logger::Logger*
Frame::getLogger()
{
    return logger_;
}

std::string
Frame::doToString() const
{
    std::stringstream s;
    for(int i = 0; i < numberOfSubChannels_; i++)
        s << !rbs_.at(i).isFree();
    return s.str();
}

ResourceGrid::ResourceGrid(const wns::pyconfig::View& config, 
        wns::logger::Logger& logger,
        unsigned int numberOfFrames, 
        unsigned int subChannels) :
    logger_(&logger),
    numberOfFrames_(numberOfFrames),
    subChannelsPerFrame_(subChannels)
{
    assure(numberOfFrames_ > 0, "Need more than zero frames");

    for(int i = 0; i < numberOfFrames_; i++)
    {
        frames_.push_back(Frame(this, i));
    }
}

ResourceGrid::~ResourceGrid()
{
}

unsigned int
ResourceGrid::getSubChannelsPerFrame()
{
    return subChannelsPerFrame_;
}

unsigned int
ResourceGrid::getNumberOfFrames()
{
    return numberOfFrames_;
}

bool
ResourceGrid::scheduleCID(unsigned int frame, ConnectionID cid, 
    unsigned int length, bool persistent)
{
    assure(frame < numberOfFrames_, "Invalid frame index.");

    Frame::SearchResultSet srs;
    srs = frames_[frame].findTransmissionBlocks(length);

    if(!srs.empty())
    {
        MESSAGE_SINGLE(NORMAL, *logger_, "Found: " << srs.size() 
            << " potential TBs. Reserving first.");

        frames_[frame].reserve(cid, srs.begin()->start, length, persistent);
        return true;
    }
    else
    {
        MESSAGE_SINGLE(NORMAL, *logger_, "Could not find " << length 
            << " resources for CID " << cid);
    }
    return false;
}

Frame*
ResourceGrid::getFrame(unsigned int index)
{
    assure(index < numberOfFrames_, "Frame index out of range");
    
    return &frames_[index];
}

void
ResourceGrid::onNewFrame(unsigned int index)
{
    assure(index < numberOfFrames_, "Frame index out of range");
    
    MESSAGE_SINGLE(NORMAL, *logger_, "Clearing unpersistent schedule of frame " << index);

    frames_[index].clearUnpersistentSchedule();
}

wns::logger::Logger*
ResourceGrid::getLogger()
{
    return logger_;
}


