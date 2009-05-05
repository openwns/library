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

#include <WNS/scheduler/strategy/ProportionalFairBase.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>
#include <WNS/scheduler/CallBackInterface.hpp>
#include <WNS/module/Base.hpp>

#include <vector>
#include <map>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <queue>
#include <math.h>

using namespace wns::scheduler;
using namespace wns::scheduler::strategy;


ProportionalFairBase::ProportionalFairBase(const wns::pyconfig::View& config)
	: Strategy(config),
	  historyWeight(config.get<float>("historyWeight")),
	  maxBursts(config.get<int>("maxBursts")),
	  allowReGrouping(config.get<bool>("allowReGrouping")),
	  scalingBetweenMaxTPandPFair(config.get<float>("scalingBetweenMaxTPandPFair")),
	  minRateOfSubchannel(0.0),
	  maxRateOfSubchannel(0.0),
	  parent(0),
	  preferenceVariationDistribution(NULL)
{
	pastDataRates.clear();
	preferenceVariationDistribution = new wns::distribution::Uniform(-1.0, 1.0);
}

ProportionalFairBase::~ProportionalFairBase()
{
	delete preferenceVariationDistribution;
}

void
ProportionalFairBase::onColleaguesKnown()
{
	// first-time preparations
	// (cannot be done in constructor as colleagues are not known)
	if (minRateOfSubchannel==0.0) {
		wns::service::phy::phymode::PhyModeMapperInterface *phyModeMapper = colleagues.registry->getPhyModeMapper();
		assure(phyModeMapper != NULL,"couldn't get phyModeMapper");
		wns::service::phy::phymode::PhyModeInterfacePtr phyMode = phyModeMapper->getLowestPhyMode();
		assure(phyMode->isValid(),"invalid PhyMode");
		assure(phyMode->dataRateIsValid(),"invalid PhyMode datarate");
		minRateOfSubchannel = phyMode->getDataRate();
		maxRateOfSubchannel = colleagues.registry->getPhyModeMapper()->getHighestPhyMode()->getDataRate();
		MESSAGE_SINGLE(NORMAL, logger, "ProportionalFairBase::onColleaguesKnown(): minRateOfSubchannel="<<minRateOfSubchannel<<" bit/s, maxRateOfSubchannel="<<maxRateOfSubchannel<<" bit/s");
		assure(minRateOfSubchannel>0.0, "unknown minRateOfSubchannel");
		assure(maxRateOfSubchannel>0.0, "unknown maxRateOfSubchannel");
	}
}

float
ProportionalFairBase::getPreference(Group group) const
{
	// calculate groupRate,
	// which is the maximum possible data rate
	// for one subChannel
	// for the best PhyMode available here
	DataRate groupRate = 0.0;
	for (Group::const_iterator iter = group.begin();
	     iter != group.end(); ++iter)
	{
		wns::Ratio sinr(iter->second.C / iter->second.I);
		const wns::service::phy::phymode::PhyModeInterfacePtr phyMode =
			this->getBestPhyMode(sinr);
		assure(phyMode->dataRateIsValid(),"invalid PhyMode datarate");
		double rate = phyMode->getDataRate(); // rate [b/s] of one subChannel
		groupRate += rate;
	}

	// get the past data rates for this group's users only:
	// iterate over the global past data rates map and save those that are in
	// our group
	// there is exactly one pastDataRate value per userID
	std::vector<DataRate> thisGroupPastRates; thisGroupPastRates.clear();
	for (std::map<UserID, DataRate>::const_iterator iter = pastDataRates.begin();
		 iter != pastDataRates.end(); ++iter)
	{
		if (group.find(iter->first/*userID*/) != group.end())
		{
			double dataRate = iter->second;
			// a RN must get a better share of the bandwidth
			// here: proportional to its number of users:
			int weight = colleagues.registry->getTotalNumberOfUsers(iter->first);
			MESSAGE_SINGLE(NORMAL, logger, "getPreference("<<printGroup(group)<<"): dataRate("<<colleagues.registry->getNameForUser(iter->first)<<")="<<dataRate<<" with weight="<<weight);
			dataRate /= static_cast<double>(weight);
			// dataRate now has the meaning of a weight.
			thisGroupPastRates.push_back(dataRate);
		}
	} // for all userIDs in pastDataRates

	// of all users in this group determine the minimum pastDataRate:
	DataRate minOfPastDataRates = 1.0; // bits/s
	if (thisGroupPastRates.size())
	{
		minOfPastDataRates = *(min_element(thisGroupPastRates.begin(), thisGroupPastRates.end()));
		// avoid division by zero, overflows
		if (minOfPastDataRates < 0.01)
			minOfPastDataRates = 0.01;
	}

	// if there is only one userID in the group,
	// minOfPastDataRates is equal to the pastDataRates[userid]

	// preference is achievable current group sum rate divided by a history
	// factor that takes the minimum past throughput of any group member into account

	// historyWeight:
	// if 0 no history is taken into account -> maxThroughput Scheduler

	//float result = groupRate / (1.0 + minOfPastDataRates * scalingBetweenMaxTPandPFair); // old implementation
	assure(scalingBetweenMaxTPandPFair>=0.0, "scalingBetweenMaxTPandPFair is out of bounds");
	assure(scalingBetweenMaxTPandPFair<=1.0, "scalingBetweenMaxTPandPFair is out of bounds");
	assure((historyWeight>=0.0)&&(historyWeight<1.0), "historyWeight is out of bounds");
	// minOfPastDataRates/groupRate ~~ number of used subchannels
	 // maxGroupRate is constant [bit/s]
	float resultMaxThroughput = groupRate / maxRateOfSubchannel; // [0..1]
	float resultPropFair      = groupRate / minOfPastDataRates;  // []
	if (scalingBetweenMaxTPandPFair <= 0.5) {
		// variate the preference for each user, so that they differ a little bit (1%)
		// and the automatic sorting does not always give the same order
		// (would be a problem for identical preference weights).
		resultMaxThroughput *=  (1 + 0.01*(*preferenceVariationDistribution)());
	}
	float result =
		(1.0-scalingBetweenMaxTPandPFair) * resultMaxThroughput
		    +scalingBetweenMaxTPandPFair  * resultPropFair;
	MESSAGE_SINGLE(NORMAL, logger, "getPreference("<<printGroup(group)<<"): gr="<<groupRate<<"bit/s, pdr="<<minOfPastDataRates<<"bit/s, result="<<result<<" (rMT="<<resultMaxThroughput<<",rPF="<<resultPropFair<<")");
	return result;
} // getPreference()


void
ProportionalFairBase::updatePastDataRates(UserID user, Bits dataThisFrame, simTimeType phaseLength)
{
	DataRate currentRate = float(dataThisFrame) / phaseLength;
	DataRate pastDataRate = pastDataRates[user];
	if (pastDataRates.find(user) != pastDataRates.end()) {
		pastDataRates[user] = (1.0-historyWeight) * currentRate + historyWeight * pastDataRates[user];
	} else { // new user
		pastDataRates[user] = currentRate;
	}
	MESSAGE_SINGLE(NORMAL, logger, "updatePastDataRates("<<user->getName()<<","<<phaseLength<<"s): pastDataRate: new= "<< pastDataRates[user]<<" bit/s, old= "<<pastDataRate<<" bit/s, currentRate= "<<currentRate<<" bit/s");
}

bool
ProportionalFairBase::everybodyFinished(std::map<UserID, bool> finishedUsers) const
{
	bool retVal = true;

	for (std::map<UserID, bool>::const_iterator iter = finishedUsers.begin();
		 iter != finishedUsers.end(); ++iter)
		retVal = retVal && iter->second;

	if (retVal) {
		MESSAGE_SINGLE(NORMAL, logger, "everybody finished");
	}

	return retVal;
}

UserID
ProportionalFairBase::getNextUnfinished(std::map<UserID, simTimeType> timeMarkers, std::map<UserID, bool> finishedUsers) const
{
	UserID next = 0;
	simTimeType min = 999999;

	for (std::map<UserID, simTimeType>::const_iterator iter = timeMarkers.begin();
		 iter != timeMarkers.end(); ++iter)
		if ((iter->second < min) && !finishedUsers[iter->first])
		{
			next = iter->first;
			min = iter->second;
		}

	return next;
}

ConnectionID
ProportionalFairBase::getNextConnWithDataForUser(UserID user) const
{
	// get all registered connctions for current user
	ConnectionVector allRegisteredConns = colleagues.registry->getConnectionsForUser(user);

	// search all registered connections for a queue that has some pdus to
	// schedule. We obey to the implicit prioritization of queues. The
	// convention is that lower cids are prioritized and should be emptied first

	for (ConnectionVector::iterator iter = allRegisteredConns.begin();
		 iter != allRegisteredConns.end();
		 ++iter)
		if (colleagues.queue->queueHasPDUs(*iter))
			return *iter;

	// no queue with pdus found
	return noCID;
}

bool
ProportionalFairBase::groupExhausted(Group group) const
{
	bool exhausted = false;
	// Iterate over all users in the group
	for (Group::const_iterator iter = group.begin();
	     iter != group.end(); ++iter)
	{
		UserID user = iter->first;
		// if the queue for at least one of the users has run out of
		// data consider this group exhausted
		if (colleagues.queue->numCompoundsForUser(user) == 0)
			exhausted = true;
	}
	return exhausted;
}



