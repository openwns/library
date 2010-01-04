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

#include <WNS/scheduler/queue/QueueProxy.hpp>
#include <WNS/ldk/Layer.hpp>

using namespace wns::scheduler::queue;

STATIC_FACTORY_REGISTER_WITH_CREATOR(QueueProxy,
                                     wns::scheduler::queue::QueueInterface,
                                     "wns.scheduler.queue.QueueProxy",
                                     wns::HasReceptorConfigCreator);

QueueProxy::QueueProxy(wns::ldk::HasReceptorInterface*, const wns::pyconfig::View& _config) :     
    logger_(_config.get("logger")),
    myFUN_(NULL)
{
}

QueueProxy::~QueueProxy()
{
}

void QueueProxy::setFUN(wns::ldk::fun::FUN* fun)
{
    myFUN_ = fun;
}

bool QueueProxy::isAccepting(const wns::ldk::CompoundPtr&  compound ) const 
{
}

void
QueueProxy::put(const wns::ldk::CompoundPtr& compound) 
{
}

wns::scheduler::UserSet
QueueProxy::getQueuedUsers() const 
{
}

wns::scheduler::ConnectionSet
QueueProxy::getActiveConnections() const
{
}

uint32_t
QueueProxy::numCompoundsForCid(wns::scheduler::ConnectionID cid) const
{
}

uint32_t
QueueProxy::numBitsForCid(wns::scheduler::ConnectionID cid) const
{
}

wns::scheduler::QueueStatusContainer
QueueProxy::getQueueStatus() const
{
}

wns::ldk::CompoundPtr
QueueProxy::getHeadOfLinePDU(wns::scheduler::ConnectionID cid) 
{
}

int
QueueProxy::getHeadOfLinePDUbits(wns::scheduler::ConnectionID cid)
{
}

bool
QueueProxy::isEmpty() const
{
}

bool
QueueProxy::hasQueue(wns::scheduler::ConnectionID cid)
{
}

bool
QueueProxy::queueHasPDUs(wns::scheduler::ConnectionID cid) 
{
}

wns::scheduler::ConnectionSet
QueueProxy::filterQueuedCids(wns::scheduler::ConnectionSet connections) 
{
}

void
QueueProxy::setColleagues(wns::scheduler::RegistryProxyInterface* registry) 
{
    colleagues.registry_ = registry;
}

wns::scheduler::queue::QueueInterface::ProbeOutput
QueueProxy::resetAllQueues()
{
    // Store number of bits and compounds for Probe which will be deleted
/*    ProbeOutput probeOutput;
        
    probeOutput.bits += iter->second.bits;
    probeOutput.compounds += iter->second.pduQueue.size();

    return probeOutput;*/
}

wns::scheduler::queue::QueueInterface::ProbeOutput
QueueProxy::resetQueues(wns::scheduler::UserID _user)
{
//    return probeOutput;
}

wns::scheduler::queue::QueueInterface::ProbeOutput
QueueProxy::resetQueue(wns::scheduler::ConnectionID cid)
{
//    return probeOutput;
}

std::string
QueueProxy::printAllQueues()
{
/*    std::stringstream s;
        ConnectionID cid = iter->first;
        int bits      = iter->second.bits;
        int compounds = iter->second.pduQueue.size();
        s << cid << ":" << bits << "," << compounds << " ";
    }
    return s.str();*/
}

bool
QueueProxy::supportsDynamicSegmentation() const
{
}

wns::ldk::CompoundPtr 
QueueProxy::getHeadOfLinePDUSegment(wns::scheduler::ConnectionID cid, int bits)
{
}

int 
QueueProxy::getMinimumSegmentSize() const
{
}



