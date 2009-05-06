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

#include <WNS/scheduler/strategy/dsastrategy/LinearFFirst.hpp>
#include <WNS/scheduler/strategy/dsastrategy/DSAStrategy.hpp>
#include <WNS/scheduler/strategy/dsastrategy/DSAStrategyInterface.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>
#include <vector>
#include <iostream>
#include <algorithm>

using namespace wns::scheduler;
using namespace wns::scheduler::strategy;
using namespace wns::scheduler::strategy::dsastrategy;

STATIC_FACTORY_REGISTER_WITH_CREATOR(LinearFFirst,
				     DSAStrategyInterface,
				     "LinearFFirst",
				     wns::PyConfigViewCreator);

LinearFFirst::LinearFFirst(const wns::pyconfig::View& config)
	: DSAStrategy(config),
	  randomDist(NULL),
	  useRandomChannelAtBeginning(false),
	  lastUsedSubChannel(0),
	  lastUsedBeam(0)
{
  useRandomChannelAtBeginning = config.get<bool>("useRandomChannelAtBeginning");
  if (useRandomChannelAtBeginning)
    randomDist = new wns::distribution::StandardUniform();
}

LinearFFirst::~LinearFFirst()
{
	if (randomDist!=NULL) delete randomDist;
}

// call this before each timeSlot/frame
void
LinearFFirst::initialize(SchedulerStatePtr schedulerState,
			 SchedulingMapPtr schedulingMap)
{
	DSAStrategy::initialize(schedulerState,schedulingMap); // must always initialize base class too
	lastUsedSubChannel = 0;
	if (useRandomChannelAtBeginning)
	{
	  int maxSubChannel = schedulingMap->subChannels.size();
	  double random = (*randomDist)();
	  lastUsedSubChannel = random*maxSubChannel;
	}
}

DSAResult
LinearFFirst::getSubChannelWithDSA(RequestForResource& request,
				   SchedulerStatePtr schedulerState,
				   SchedulingMapPtr schedulingMap)
{
	DSAResult dsaResult;
	//simTimeType requestedCompoundDuration = getCompoundDuration(request);
	//MESSAGE_SINGLE(NORMAL, logger, "getSubChannelWithDSA("<<request.toString()<<"): d="<<requestedCompoundDuration<<"s");
	int subChannel = lastUsedSubChannel;
	int beam = lastUsedBeam;
	int maxSubChannel = schedulerState->currentState->strategyInput->getFChannels();
	int maxBeams = schedulerState->currentState->strategyInput->getMaxBeams();
	assure(subChannel<maxSubChannel,"invalid subChannel="<<subChannel);
	MESSAGE_SINGLE(NORMAL, logger, "getSubChannelWithDSA("<<request.toString()<<"): lastSC="<<lastUsedSubChannel);
	assure(maxSubChannel==schedulerState->currentState->strategyInput->getFChannels(),"maxSubChannel="<<maxSubChannel<<"!=fChannels");
	bool found  = false;
	bool giveUp = false;
	while(!found && !giveUp) {
	    if (channelIsUsable(subChannel, beam, request, schedulerState, schedulingMap))
	    { // PDU fits in
	      found=true; break;
	    }
	    if (++beam>=maxBeams)
	    { // all beams full; take next subChannel
	      beam=0;
	      if (++subChannel>=maxSubChannel)
	      { // wraparound
		  subChannel=0;
	      }
	    }
	    if (subChannel==lastUsedSubChannel)
	    { // one complete round already done
	      giveUp=true; break;
	    }
	} // while
	if (giveUp) {
	  MESSAGE_SINGLE(NORMAL, logger, "getSubChannelWithDSA(): no free subchannel");
	  return dsaResult; // empty with subChannel=DSAsubChannelNotFound
	} else {
	  MESSAGE_SINGLE(NORMAL, logger, "getSubChannelWithDSA(): subChannel="<<subChannel);
	  lastUsedSubChannel = subChannel;
	  lastUsedBeam = beam;
	  dsaResult.subChannel = subChannel;
	  dsaResult.beam = beam;
	  return dsaResult;
	}
}
