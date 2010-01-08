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
    queueManagerServiceName_(_config.get<std::string>("queueManagerServiceName")),
    readOnly_(_config.get<bool>("readOnly")),
    logger_(_config.get("logger")),
    myFUN_(NULL)
{
    MESSAGE_BEGIN(NORMAL, logger_, m, "QueueProxy");
    m << " Created ";
    m << (readOnly_?"read only":"read/write"); 
    m << " QueueProxy Queue using QueueManagerService ";
    m << queueManagerServiceName_;
    MESSAGE_END();

}

QueueProxy::~QueueProxy()
{
}

void QueueProxy::setFUN(wns::ldk::fun::FUN* fun)
{
    myFUN_ = fun;
    colleagues.queueManager_ = fun->getLayer()->
            getManagementService<wns::scheduler::queue::IQueueManager>(
                queueManagerServiceName_);
    assure(colleagues.queueManager_, "QueueProxy needs a QueueManager");

    MESSAGE_BEGIN(NORMAL, logger_, m, myFUN_->getName());
    m << " Received valid FUN pointer and QueueManagerService ";
    m << queueManagerServiceName_;
    MESSAGE_END();
}

bool QueueProxy::isAccepting(const wns::ldk::CompoundPtr&  compound ) const 
{
    if(readOnly_)
        return false;

    assure(false, "QueueProxy can currently only be used read only");

}

void
QueueProxy::put(const wns::ldk::CompoundPtr& compound) 
{
    assure(!readOnly_, "Put called for readOnly QueueProxy");
    
    if(readOnly_)
        return;
}

wns::scheduler::UserSet
QueueProxy::getQueuedUsers() const 
{
    wns::scheduler::UserSet us;

    QueueContainer queues = colleagues.queueManager_->getAllQueues();    

    QueueContainer::iterator it;
    for(it = queues.begin(); it != queues.end(); it++)
    {
        wns::scheduler::UserSet innerUs;
        innerUs = it->second->getQueuedUsers();
        wns::scheduler::UserSet::iterator iit;
        
        for(iit = innerUs.begin(); iit != innerUs.end(); iit++)
            us.insert(*iit);
    }
    return us;

}

wns::scheduler::ConnectionSet
QueueProxy::getActiveConnections() const
{
    wns::scheduler::ConnectionSet cs;

    QueueContainer queues = colleagues.queueManager_->getAllQueues();    

    QueueContainer::iterator it;
    for(it = queues.begin(); it != queues.end(); it++)
    {
        wns::scheduler::ConnectionSet innerCs;
        innerCs = it->second->getActiveConnections();
        wns::scheduler::ConnectionSet::iterator iit;
        
        for(iit = innerCs.begin(); iit != innerCs.end(); iit++)
            cs.insert(*iit);
    }
    return cs;
}

uint32_t
QueueProxy::numCompoundsForCid(wns::scheduler::ConnectionID cid) const
{
    assure(colleagues.queueManager_->getQueue(cid) != NULL, "No queue for this CID");
    return colleagues.queueManager_->getQueue(cid)->numCompoundsForCid(cid);  
}

uint32_t
QueueProxy::numBitsForCid(wns::scheduler::ConnectionID cid) const
{
    assure(colleagues.queueManager_->getQueue(cid) != NULL, "No queue for this CID");
    return colleagues.queueManager_->getQueue(cid)->numBitsForCid(cid);   
}

wns::scheduler::QueueStatusContainer
QueueProxy::getQueueStatus() const
{
    wns::scheduler::QueueStatusContainer csc;

    QueueContainer queues = colleagues.queueManager_->getAllQueues();    

    QueueContainer::iterator it;
    for(it = queues.begin(); it != queues.end(); it++)
    {
        wns::scheduler::QueueStatusContainer innerCsc;
        innerCsc = it->second->getQueueStatus();
        wns::scheduler::QueueStatusContainer::const_iterator iit;
        
        for(iit = innerCsc.begin(); iit != innerCsc.end(); iit++)
            csc.insert(iit->first, iit->second);
    }
    return csc;
}

wns::ldk::CompoundPtr
QueueProxy::getHeadOfLinePDU(wns::scheduler::ConnectionID cid) 
{        
    assure(copyQueue_.find(cid) != copyQueue_.end(), "No copyQueue for CID");
    assure(!copyQueue_[cid].empty(), "Requested PDU from emty queue");
    
    wns::ldk::CompoundPtr pdu = copyQueue_[cid].front();        
    copyQueue_[cid].pop();        
    return pdu;
}

int
QueueProxy::getHeadOfLinePDUbits(wns::scheduler::ConnectionID cid)
{
    assure(hasQueue(cid), "No queue for this CID");
    return colleagues.queueManager_->getQueue(cid)->getHeadOfLinePDUbits(cid);
}

bool
QueueProxy::isEmpty() const
{
    QueueContainer queues = colleagues.queueManager_->getAllQueues();    
    QueueContainer::iterator it;
    for(it = queues.begin(); it != queues.end(); it++)
    {
        if(!(it->second->isEmpty()))
            return false;
    }
    return true;
}

bool
QueueProxy::hasQueue(wns::scheduler::ConnectionID cid)
{
    wns::scheduler::queue::QueueInterface* queue;
    queue = colleagues.queueManager_->getQueue(cid);

    return (queue == NULL)?false:true;
}

bool
QueueProxy::queueHasPDUs(wns::scheduler::ConnectionID cid) 
{
    wns::scheduler::queue::QueueInterface* queue;
    queue = colleagues.queueManager_->getQueue(cid);

    if(queue != NULL)
    {
        createQueueCopyIfNeeded(cid);

        if(copyQueue_.find(cid) != copyQueue_.end())
        {
            if(copyQueue_[cid].empty()) 
            {
                MESSAGE_BEGIN(NORMAL, logger_, m, myFUN_->getName());
                m << " queueHasPDUs: CopyQueue for CID " << cid << " is empty.";
                MESSAGE_END();

                return false;
            }
            else
            {
                return true;
            }
        }
        else
        {
            std::cout << "Pass: "<< queue->queueHasPDUs(cid) << "\n";
            MESSAGE_BEGIN(NORMAL, logger_, m, myFUN_->getName());
            m << " queueHasPDUs: Passing call for  CID " << cid << " to real queue.";
            MESSAGE_END();

            return queue->queueHasPDUs(cid);
        }
    }
    else
    {
        return false;
    }
}

wns::scheduler::ConnectionSet
QueueProxy::filterQueuedCids(wns::scheduler::ConnectionSet connections) 
{
    colleagues.queueManager_->getAllQueues();    
}

void
QueueProxy::setColleagues(wns::scheduler::RegistryProxyInterface* registry) 
{
    colleagues.registry_ = registry;
}

wns::scheduler::queue::QueueInterface::ProbeOutput
QueueProxy::resetAllQueues()
{
    wns::scheduler::queue::QueueInterface::ProbeOutput po;

    QueueContainer queues = colleagues.queueManager_->getAllQueues();    

    QueueContainer::iterator it;
    for(it = queues.begin(); it != queues.end(); it++)
    {
        wns::scheduler::queue::QueueInterface::ProbeOutput innerPo;
        innerPo = it->second->resetAllQueues();
        po.bits += innerPo.bits;
        po.compounds += innerPo.compounds;
    }
    return po;   
}

wns::scheduler::queue::QueueInterface::ProbeOutput
QueueProxy::resetQueues(wns::scheduler::UserID _user)
{
    assure(false, "Not implemeted, use request with CID instead");
}

wns::scheduler::queue::QueueInterface::ProbeOutput
QueueProxy::resetQueue(wns::scheduler::ConnectionID cid)
{
    assure(hasQueue(cid), "No queue for this CID");
    return colleagues.queueManager_->getQueue(cid)->resetQueue(cid);    
}

std::string
QueueProxy::printAllQueues()
{
    std::stringstream s;
    
    QueueContainer queues = colleagues.queueManager_->getAllQueues();    

    QueueContainer::iterator it;
    for(it = queues.begin(); it != queues.end(); it++)
    {    
        s << it->second->printAllQueues() << "\n";
    }
    return s.str();
}

bool
QueueProxy::supportsDynamicSegmentation() const
{
    return false;   
}

wns::ldk::CompoundPtr 
QueueProxy::getHeadOfLinePDUSegment(wns::scheduler::ConnectionID cid, int bits)
{
}

int 
QueueProxy::getMinimumSegmentSize() const
{
}

void
QueueProxy::createQueueCopyIfNeeded(wns::scheduler::ConnectionID cid)
{
    wns::simulator::Time now = wns::simulator::getEventScheduler()->getTime();

    // New round, create new PDUs in copyQueue
    if(lastChecked_.find(cid) == lastChecked_.end() || lastChecked_[cid] != now)
    {
        // Empty the old copy queue
        if(copyQueue_.find(cid) != copyQueue_.end())
        {
            MESSAGE_BEGIN(NORMAL, logger_, m, myFUN_->getName());
            m << " Removing " << copyQueue_[cid].size() << " PDUs form old copyQueue ";
            m << " for CID " << cid;
            MESSAGE_END();

            copyQueue_[cid] = std::queue<wns::ldk::CompoundPtr>();
        }
            
        wns::scheduler::queue::QueueInterface* queue;
        queue = colleagues.queueManager_->getQueue(cid);
        //uint32_t pdus = queue->numCompoundsForCid(cid);
        
        copyQueue_[cid] = queue->getQueueCopy(cid);

        MESSAGE_BEGIN(NORMAL, logger_, m, myFUN_->getName());
        m << " Creating copy of " << copyQueue_[cid].size() << " PDUs for CID ";
        m << cid;
        MESSAGE_END();

        //for(uint32_t i = 0; i < pdus; i++)
        //{
        //    wns::ldk::CompoundPtr pdu = colleagues.queueManager_->getQueue(cid)->getHeadOfLinePDU(cid);

            // TODO: Use FakePDUs instead!    
        //    copyQueue_[cid].push(pdu->copy());
        //    queue->put(pdu);
        //}
        lastChecked_[cid] = now;
    }
}

std::queue<wns::ldk::CompoundPtr> 
QueueProxy::getQueueCopy(ConnectionID cid)
{ 
    wns::Exception("You should not call getQueueCopy of the QueueProxy."); 
}

