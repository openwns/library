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

#include <WNS/scheduler/SchedulerTypes.hpp>
#include <WNS/scheduler/strategy/staticpriority/SubStrategy.hpp>

#include <vector>
#include <map>

using namespace wns::scheduler;
using namespace wns::scheduler::strategy;
using namespace wns::scheduler::strategy::staticpriority;


SubStrategy::SubStrategy()
  : logger(std::string("WNS"), std::string("SubStrategy")),
    colleagues(),
    useDynamicSegmentation(false),
    minimumSegmentSize(-1)
{
}

SubStrategy::SubStrategy(const wns::pyconfig::View& config)
  : logger(config.get("logger")),
    colleagues(),
    useDynamicSegmentation(false),
    minimumSegmentSize(-1)
{
}

void
SubStrategy::setColleagues(wns::scheduler::strategy::Strategy* _strategy,
			   wns::scheduler::queue::QueueInterface* _queue,
			   wns::scheduler::RegistryProxyInterface* _registry)
{
	colleagues.strategy = _strategy;
	colleagues.queue = _queue;
	colleagues.registry = _registry;
	assure(colleagues.strategy!=NULL, "Need access to the strategy");
	assure(dynamic_cast<queue::QueueInterface*>(colleagues.queue), "Need access to the queue");
	assure(dynamic_cast<RegistryProxyInterface*>(colleagues.registry), "Need access to the registry");
	useDynamicSegmentation = colleagues.queue->supportsDynamicSegmentation();
	if (useDynamicSegmentation) {
	  MESSAGE_SINGLE(NORMAL, logger, "WARNING: NEW useDynamicSegmentation="<<useDynamicSegmentation);
	  minimumSegmentSize = colleagues.queue->getMinimumSegmentSize();
	}
	this->initialize();
}

bool
SubStrategy::scheduleCid(SchedulerStatePtr schedulerState,
			 wns::scheduler::SchedulingMapPtr schedulingMap,
			 const wns::scheduler::ConnectionID cid,
			 int& pduCounter, // modified
			 const int blockSize,
			 MapInfoCollectionPtr mapInfoCollection // result
			 )
{
  assure(pduCounter<blockSize,"required: pduCounter="<<pduCounter<<" < blockSize="<<blockSize);
  UserID userID = colleagues.registry->getUserForCID(cid);
  int queuedBits = colleagues.queue->getHeadOfLinePDUbits(cid);
  int requestedBits =
    (useDynamicSegmentation && (queuedBits>minimumSegmentSize)) ?
    minimumSegmentSize // ask for any free space of at least this size
    :
    queuedBits;
  MESSAGE_SINGLE(NORMAL, logger, "scheduleCid(CID="<<cid<<" of "<<userID->getName()<<",#"<<pduCounter<<"): bits: "<<queuedBits<<" queued, "<<requestedBits<<" requested");

  // do resource scheduling here:
  RequestForResource request(cid,userID,queuedBits);
  MapInfoEntryPtr mapInfoEntry =
    colleagues.strategy->doAdaptiveResourceScheduling(request, schedulingMap);
  if (mapInfoEntry == MapInfoEntryPtr()) // no result
    return false;
  // ^ maybe it could make sense to try other connections here instead of aborting?
  // ^ they could have smaller PDUs, better PhyMode or oneUserOnOneSubChannel=true

  int subChannel = mapInfoEntry->subBand;
  int beam = mapInfoEntry->beam;
  simTimeType compoundStartTime = schedulingMap->getNextPosition(subChannel, beam);
  simTimeType allCompoundsEndTime = compoundStartTime;
  assure(compoundStartTime==schedulingMap->subChannels[subChannel].physicalResources[beam].getNextPosition(),"mismatch in getNextPosition");
  mapInfoEntry->start = compoundStartTime;
  // ^ all the above is constant even if we schedule another compound of the same cid
  // this depends on phyMode and subChannel used so far:
  int freeBits = schedulingMap->getFreeBitsOnSubChannel(mapInfoEntry);
  // ^ can be used with DynamicSegmentation
  MESSAGE_SINGLE(NORMAL, logger, "scheduleCid(CID="<<cid<<" of "<<userID->getName()<<"): bits: "<<queuedBits<<" queued, "<<requestedBits<<" requested, "<<freeBits<<" free(sc="<<mapInfoEntry->subBand<<")");

  if (useDynamicSegmentation) {
    // TODO
    wns::ldk::CompoundPtr compoundPtr = colleagues.queue->getHeadOfLinePDUSegment(cid,freeBits);
    schedulingMap->addCompound(request, mapInfoEntry, compoundPtr);
    mapInfoEntry->compounds.push_back(compoundPtr); // (currentBurst)
    allCompoundsEndTime += request.getDuration();
    throw wns::Exception("useDynamicSegmentation: not yet implemented");
  } else { // normal pre-segmented PDUs
    //while(schedulingMap->pduFitsIntoSubChannel(request, mapInfoEntry))
    if (request.bits > freeBits) {
      MESSAGE_SINGLE(NORMAL, logger, "scheduleCid(CID="<<cid<<" of "<<userID->getName()<<"): request.bits="<<request.bits<<" do not fit into free "<<freeBits<<" bits.");
      return false;
    }

    // loop to put more pdus into the resource block if possible
    while (request.bits <= freeBits)
    { // try to put one (or more) pdus into this resource block
      wns::ldk::CompoundPtr compoundPtr = colleagues.queue->getHeadOfLinePDU(cid);
      MESSAGE_SINGLE(NORMAL, logger, "scheduleCid(CID="<<cid<<" of "<<userID->getName()<<"): request.bits="<<request.bits);
      schedulingMap->addCompound(request, mapInfoEntry, compoundPtr);
      mapInfoEntry->compounds.push_back(compoundPtr); // (currentBurst)
      allCompoundsEndTime += request.getDuration();
      pduCounter++;
      freeBits -= request.bits;
      // another one?
      if (colleagues.queue->queueHasPDUs(cid)) {
	request.bits = colleagues.queue->getHeadOfLinePDUbits(cid);
	// mapInfoEntry can be left unchanged (contains only invariants)
      } else {
	break; // no more pdu in queue[cid]
      }
    } // while still space on subChannel

  } // no DynamicSegmentation
  mapInfoEntry->end = allCompoundsEndTime;
  mapInfoCollection->push_back(mapInfoEntry);
  MESSAGE_SINGLE(NORMAL, logger, "scheduleCid(CID="<<cid<<" of "<<userID->getName()<<"): next PDU on next subChannel...?");
  return true; // true means success
} // scheduleCid
