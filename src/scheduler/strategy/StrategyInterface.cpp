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

#include <WNS/scheduler/CallBackInterface.hpp>
#include <WNS/scheduler/strategy/StrategyInterface.hpp>

using namespace wns::scheduler;
using namespace wns::scheduler::strategy;

// interface for master scheduling (old interface)
StrategyInput::StrategyInput(int _fChannels,
			     double _slotLength,
			     int _maxBeams,
			     CallBackInterface* _callBackObject)
  : fChannels(_fChannels),
    slotLength(_slotLength),
    beamforming(_maxBeams>1), // in the old strategies we assume "beamforming" if maxBeams>1.
    maxBeams(_maxBeams),
    callBackObject(_callBackObject),
    mapInfoEntryFromMaster(), // empty SmartPtr
    frameNr(-1)
{
  // these two checks are equivalent:
  assure(mapInfoEntryFromMaster.getPtr()==NULL,"SmartPtr must be initialized with NULL");
  assure(mapInfoEntryFromMaster==MapInfoEntryPtr(),"SmartPtr must be initialized with NULL");
}

// interface for slave scheduling
StrategyInput::StrategyInput(MapInfoEntryPtr _mapInfoEntryFromMaster,
			     CallBackInterface* _callBackObject)
  : fChannels(1),
    slotLength(_mapInfoEntryFromMaster->end - _mapInfoEntryFromMaster->start),
    beamforming(false),
    maxBeams(1),
    callBackObject(_callBackObject),
    mapInfoEntryFromMaster(_mapInfoEntryFromMaster), // SmartPtr
    frameNr(-1)
{
  // these two checks are equivalent:
  assure(mapInfoEntryFromMaster.getPtr()!=NULL,"SmartPtr must be valid");
  assure(mapInfoEntryFromMaster!=MapInfoEntryPtr(),"SmartPtr must be valid");
}

// generic interface for master or slave scheduling
StrategyInput::StrategyInput(int _fChannels,
			     double _slotLength,
			     int _maxBeams,
			     MapInfoEntryPtr _mapInfoEntryFromMaster,
			     CallBackInterface* _callBackObject)
  : fChannels(_fChannels),
    slotLength(_slotLength),
    beamforming(_maxBeams>1), // in the old strategies we assume "beamforming" if maxBeams>1.
    maxBeams(_maxBeams),
    callBackObject(_callBackObject),
    mapInfoEntryFromMaster(_mapInfoEntryFromMaster), // SmartPtr
    frameNr(-1)
{
}

StrategyInput::~StrategyInput()
{
}

// set optional parameter:
void
StrategyInput::setFrameNr(int _frameNr)
{
  assure(_frameNr>=0,"invalid frameNr="<<frameNr);
  frameNr=_frameNr;
}

bool
StrategyInput::frameNrIsValid() const
{
  return (frameNr>=0) ? true:false;
}

void
StrategyInput::setDefaultPhyMode(wns::service::phy::phymode::PhyModeInterfacePtr _phyModePtr)
{
  defaultPhyModePtr = _phyModePtr; //_phyModePtr->clone();
}

void
StrategyInput::setDefaultTxPower(wns::Power _txPower)
{
  defaultTxPower = _txPower;
}

std::string
StrategyInput::toString() const
{
  std::stringstream s;
  s << "StrategyInput():";
  s << "\tfChannels="<<fChannels<<std::endl;
  s << "\tslotLength="<<slotLength<<std::endl;
  s << "\tmaxBeams="<<maxBeams<<std::endl;
  s << "\tcallBackObject="<<callBackObject<<std::endl;
  //s << "\t"<<std::endl;
  return s.str();
};

StrategyResult::StrategyResult(wns::scheduler::SchedulingMapPtr _schedulingMap,
			       MapInfoCollectionPtr _bursts)
  : schedulingMap(_schedulingMap),
    bursts(_bursts)
{
}

StrategyResult::~StrategyResult()
{
  // members are SmartPtr's. No need to care.
}

int
StrategyResult::getNumberOfCompoundsInBursts() const
{
  int count=0;
  for ( MapInfoCollection::const_iterator iterBurst = bursts->begin();
	iterBurst != bursts->end(); ++iterBurst)
  {
    count += (*iterBurst)->compounds.size();
  }
  return count;
}

void
StrategyResult::deleteCompoundsInBursts()
{
  for ( MapInfoCollection::const_iterator iterBurst = bursts->begin();
	iterBurst != bursts->end(); ++iterBurst)
  {
    (*iterBurst)->compounds.clear();
  }
}

std::string
StrategyResult::toString() const
{
  std::stringstream s;
  s << "StrategyResult():";
  return s.str();
};

float
StrategyResult::getResourceUsage() const
{
	return 0.0;
	// TODO: implement a simple iteration over the bursts
}


/* old interface (wrapper to support; soon obsolete) */
// perform master scheduling
void
StrategyInterface::startScheduling(int fChannels,
				   int maxBeams,
				   double slotLength,
				   CallBackInterface* parent)
{
  StrategyInput strategyInput(fChannels, slotLength, maxBeams, MapInfoEntryPtr(), parent);
  startScheduling(strategyInput);
}

// perform slave scheduling
void
StrategyInterface::startScheduling(MapInfoEntryPtr burst,
				   CallBackInterface* parent)
{
  double slotLength = burst->end - burst->start;
  StrategyInput strategyInput(1, slotLength, 1, burst, parent);
  startScheduling(strategyInput);
}

/* old interface
// perform slave scheduling
void
StrategyInterface::startScheduling(MapInfoEntryPtr burst,
				   CallBackInterface* parent)
{
  //this->setPhyModePtr(burst->phyModePtr.getPtr());
	this->setPhyModePtr(burst->phyModePtr);
	this->setMasterBurst(burst);
	this->setCallBack(parent);
	int fChannels = 1;
	int maxBeams = 1;
	double slotLength = burst->end - burst->start;
	this->doStartScheduling(fChannels, maxBeams, slotLength);
	// ^ return value not used
}
*/


