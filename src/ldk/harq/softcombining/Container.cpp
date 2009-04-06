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

#include <WNS/ldk/harq/softcombining/Container.hpp>

using namespace wns::ldk::harq::softcombining;

Container::Container(int numRVs)
{
    receivedCompounds_.resize(numRVs);
}

void
Container::clear()
{
    int size = receivedCompounds_.size();
    receivedCompounds_.clear();
    receivedCompounds_.resize(size);
}

int
Container::getNumRVs() const
{
    return receivedCompounds_.size();
}

Container::CompoundList
Container::getCompoundsForRV(int rv) const
{
    checkIfValidRV(rv);

    return receivedCompounds_[rv];
}

void
Container::appendCompoundForRV(int rv, wns::ldk::CompoundPtr compound)
{
    checkIfValidRV(rv);

    receivedCompounds_[rv].push_back(compound);
}

void
Container::checkIfValidRV(int rv) const
{
    if (rv < 0 || rv >= getNumRVs())
    {
        throw Container::InvalidRV();
    }
}
