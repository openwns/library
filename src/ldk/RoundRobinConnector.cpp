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

#include <WNS/ldk/RoundRobinConnector.hpp>
#include <WNS/ldk/FunctionalUnit.hpp>

using namespace wns::ldk;

bool
RoundRobinConnector::hasAcceptor(const CompoundPtr& compound)
{
	if(fus.size() == 1) {
		return fus.current()->isAccepting(compound);
	}

	bool result = false;

	fus.startRound();
	while(fus.hasNext()) {
		FunctionalUnit* it = fus.next();

		if(it->isAccepting(compound)) {
			result = true;
			break;
		}
	}
	fus.cancelRound();
	return result;
} // hasAcceptor


CompoundHandlerInterface*
RoundRobinConnector::getAcceptor(const CompoundPtr& compound)
{
	assure(!fus.empty(), "Called getAcceptor even though no FU available");

	if(fus.size() == 1) {
		assure(fus.current()->isAccepting(compound), "Called getAcceptor although no FU is accepting");
		return fus.current();
	}

	FunctionalUnit* candidate = NULL;

	fus.startRound();
	while(fus.hasNext()) {
		candidate = fus.next();
		if(candidate->isAccepting(compound)) {
			break;
		} else {
			// not a candidate if not accpeting
			candidate = NULL;
		}
	}
	fus.endRound();

	assure(candidate, "getAcceptor call, but there is no valid candidate.");
	return candidate;
} // getAcceptor



