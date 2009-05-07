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

#include <WNS/scheduler/grouper/TrivialGrouper.hpp>

using namespace wns::scheduler;
using namespace wns::scheduler::grouper;


STATIC_FACTORY_REGISTER_WITH_CREATOR(TrivialGrouper, GroupingProviderInterface, "TrivialGrouper", wns::PyConfigViewCreator);

TrivialGrouper::TrivialGrouper(const wns::pyconfig::View& config)
	: SpatialGrouper(config)
{

}

Grouping
TrivialGrouper::getTxGrouping(const UserSet activeUsers, int /* maxBeams */) {

	assure(dynamic_cast<wns::service::phy::ofdma::DataTransmission*>(friends.ofdmaProvider),
	       "Grouper needs an OFDMA Provider");

	Grouping grouping;
	grouping.groups.clear();
	int n = 0;

	if(beamforming){

		for (UserSet::iterator iter = activeUsers.begin();
		     iter != activeUsers.end();
		     ++iter) {
			UserID user = *iter;
			assure(user, "No valid user");

			// make the one-user map for the SINR calculation
			std::map<UserID, wns::Power> map;
			map[user] = colleagues.registry->estimateTxSINRAt(user).I;

			grouping.groups.push_back(friends.ofdmaProvider->calculateCandIsTx(map, x_friendliness, txPower));
			grouping.userGroupNumber[user] = n;

			std::vector<UserID> undesireds;
			undesireds.clear();
			grouping.patterns[user] = friends.ofdmaProvider->calculateAndSetBeam(user, undesireds, x_friendliness);
			assure(grouping.patterns[user] != wns::service::phy::ofdma::PatternPtr(), "Invalid pattern returned");
			n++;
		}
	}
	else{ // no beamforming

		for (UserSet::iterator iter = activeUsers.begin();
		     iter != activeUsers.end();
		     ++iter) {
			UserID user = *iter;
			assure(user, "No valid user");

			std::map<UserID, wns::CandI> candis;
			candis[user] = colleagues.registry->estimateTxSINRAt(user);
			grouping.groups.push_back(candis);
			grouping.userGroupNumber[user] = n;
			grouping.patterns[user] = wns::service::phy::ofdma::PatternPtr();
			n++;
		}
	}
	return grouping;
}

Grouping TrivialGrouper::getRxGrouping(const UserSet activeUsers , int /* maxBeams */) {

	assure(dynamic_cast<wns::service::phy::ofdma::DataTransmission*>(friends.ofdmaProvider),
	       "Grouper needs an OFDMA Provider");

	Grouping grouping;
	grouping.groups.clear();
	int n = 0;

	if(beamforming){

		for (UserSet::iterator iter = activeUsers.begin();
		     iter != activeUsers.end();
		     ++iter) {
			UserID user = *iter;
			assure(user, "No valid user");

			// make the one-user vector for the SINR calculation
			std::vector<UserID> combination;
			combination.clear();
			combination.push_back(user);

			grouping.groups.push_back(
				friends.ofdmaProvider->calculateCandIsRx(combination,
									 colleagues.registry->estimateRxSINROf(user).I));
			grouping.userGroupNumber[user] = n;

			std::vector<UserID> undesireds;
			undesireds.clear();
			grouping.patterns[user] = friends.ofdmaProvider->calculateAndSetBeam(user, undesireds, colleagues.registry->estimateRxSINROf(user).I);
			assure(grouping.patterns[user] != wns::service::phy::ofdma::PatternPtr(), "Invalid pattern returned");
			n++;
		}
	}
	else{ // no beamforming

		for (UserSet::iterator iter = activeUsers.begin();
		     iter != activeUsers.end();
		     ++iter) {
			UserID user = *iter;
			assure(user, "No valid user");

			std::map<UserID, wns::CandI> candis;
			candis[user] = colleagues.registry->estimateRxSINROf(user);
			grouping.groups.push_back(candis);
			grouping.userGroupNumber[user] = n;
			grouping.patterns[user] = wns::service::phy::ofdma::PatternPtr();
			n++;
		}
	}
	return grouping;
}



