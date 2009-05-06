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

#include <WNS/scheduler/MapInfoEntry.hpp>
#include <WNS/node/Interface.hpp>
#include <iostream>

using namespace wns::scheduler;

MapInfoEntry::MapInfoEntry() :
	start(0.0),
	end(0.0),
	user(NULL),
	frameNr(-1),
	subBand(-1),
	beam(0),
	txPower(),
	phyModePtr(),
	estimatedCandI(),
	compounds()
{
}

MapInfoEntry::~MapInfoEntry()
{
	compounds.clear();
}

std::string
MapInfoEntry::toString() const
{
  std::stringstream s;
  s.setf(std::ios::fixed,std::ios::floatfield);   // floatfield set to fixed
  s.precision(1);
  s << "\n\t frameNr:      " << frameNr
    << "\n\t subBand:      " << subBand
    << "\n\t beam:         " << beam
    << "\n\t timespan:     [" << start*1e6 << "-" << end*1e6 << "]us";
  s << "\n\t duration:     " << (end-start)*1e6 << "us";
  if (phyModePtr!=wns::service::phy::phymode::PhyModeInterfacePtr()
      && (phyModePtr->dataRateIsValid())) {
    double dataRate = phyModePtr->getDataRate();
    int bits = dataRate*(end-start);
    s << " => space for "<<bits<<" bits";
  }
  if (phyModePtr!=wns::service::phy::phymode::PhyModeInterfacePtr()) {
    s << "\n\t phyMode:      " << *phyModePtr;
  } else {
    s << "\n\t phyMode:      undefined";
  }
  if (user!=NULL) {
    s << "\n\t userID:       " << user->getName();
  } else {
    s << "\n\t userID:       undefined";
  }
  if (txPower!=wns::Power()) {
    s << "\n\t txPower:      " << txPower;
  } else {
    s << "\n\t txPower:      undefined";
  }
  if (estimatedCandI.isValid()) {
    s << "\n\t est_CandI:    " << estimatedCandI.toSINR();
  } else {
    s << "\n\t est_CandI:    undefined";
  }
  s << "\n\t #compounds:   " << compounds.size();
    if ( !compounds.empty() ) {
      int c=1;
      for ( std::list<wns::ldk::CompoundPtr>::const_iterator iterCompound = compounds.begin();
	    iterCompound != compounds.end(); ++iterCompound){
	s << "\n\t #" << c++ << ": ";
	s << (*iterCompound)->getLengthInBits() << " bits";
      }
    }
    s << "\n";
  return s.str();
} // toString()

std::string
MapInfoEntry::doToString() const
{
  return toString();
}

void
MapInfoCollection::join(const MapInfoCollection& otherList)
{
    for(MapInfoCollectionList::const_iterator iter = otherList.begin();
	iter != otherList.end(); ++iter)
    {
	push_back(*iter);
    }
}
