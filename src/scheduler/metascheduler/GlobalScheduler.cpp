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

#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <fstream>
#include <limits.h>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

#include "GlobalScheduler.hpp"
/*
STATIC_FACTORY_REGISTER_WITH_CREATOR(MetaScheduler,
                                     IMetaScheduler,
                                     "RandomMetaScheduler",
                                     wns::PyConfigViewCreator);

*/
#include <WNS/StaticFactory.hpp>

// #define IRIDIUM_FRAME 1
#define INMARSAT_FRAME 1

#ifdef INMARSAT_FRAME
#define FRAME_LENGTH 80
#define BASE_OFFSET 0.0
#define GUARD_INTERVAL 0.0
#define SLOT_LENGTH 20.0
#else
#define FRAME_LENGTH 90
#define BASE_OFFSET 20+1.56
#define GUARD_INTERVAL 0.22
#define SLOT_LENGTH 8.28
#endif

GlobalScheduler::GlobalScheduler()
{
  //Init iridium pool
  macPool = new wns::container::Pool<int>(0,1024,1024+MAXNUMBEROFSLOTS);
  registerPool = new wns::container::Pool<int>(0,1024,1024+MAXNUMBEROFACTIVENODES);
  numberOfNodes = 0;
  periode=FRAME_LENGTH;
  numberOfTDMASlots = 4;
  //First TDMA frame starts after one periode interval

  assure((periode > 0), "GlobalScheduler called with invalid periode time.");
  assure((numberOfTDMASlots > 0), "GlobalScheduler called with invalid number of TDMA slots.");
  setNextEventTimer();
}

GlobalScheduler::GlobalScheduler(const GlobalScheduler& ){}

GlobalScheduler& GlobalScheduler::getInstance()
{
  static GlobalScheduler instance;
  return instance;
}

void GlobalScheduler::setSlotLength(double slotlength) {
  slotlength_ = slotlength;
}

void GlobalScheduler::onTimeout()
{
   NodeTimer* node;
   setNextEventTimer();
  
   for (int i=0; i<MAXNUMBEROFSLOTS ;i++)
   {
     if (nodeList[i] != NULL)
     {
       node = nodeList[i];
       node->getLower2OFDMAAddress()->onNewFrame(node->getoffset(),node->getDuration());
     }
   }
}

void GlobalScheduler::setNextEventTimer()
{	
  setTimeout(periode/1000.0);
}

void GlobalScheduler::registerNode(glue::convergence::Lower2OFDMAPhy* lower)
{
    for (int i=0; i<MAXNUMBEROFACTIVENODES ;i++) {
        if (registerList[i] == NULL) {
            registerList[i] = lower;
            break;
        }
    }
}


bool GlobalScheduler::addnode(glue::convergence::Lower2OFDMAPhy* lower)
{
    numberOfActiveNodes++;


    if (numberOfNodes < MAXNUMBEROFSLOTS)
   {
     int possibleSlot = macPool->suggestPort();
     int slotPosition = possibleSlot%numberOfTDMASlots;

     nodeList[possibleSlot-1024] = new NodeTimer(
         BASE_OFFSET+slotPosition*(slotlength_+GUARD_INTERVAL),
         slotlength_,
         ((int)((possibleSlot-1024)/numberOfTDMASlots)),
         lower);

     macPool->bind(possibleSlot);

     numberOfNodes++;
     lower->setActive(true);

     return true;
   }
  return false;
}

bool GlobalScheduler::deletenode(glue::convergence::Lower2OFDMAPhy* lower)
{
    numberOfActiveNodes--;
  // Find and delete NodeTimer object for nodeID

  for (int i = 0; i < MAXNUMBEROFSLOTS; i++)
  { 
    if(nodeList[i] != NULL)
    {
      if (nodeList[i]->getLower2OFDMAAddress() == lower)
      {
	nodeList[i]->~NodeTimer();
	nodeList[i] = NULL;
	macPool->unbind(i+1024);
        numberOfNodes--;

        lower->setActive(false);
	return true;
      }
    }
  }
  return false;
}

// NODETIMER //

GlobalScheduler::NodeTimer::NodeTimer(double offset, double duration,int fdmaChannel, glue::convergence::Lower2OFDMAPhy* lower)
{
  this->duration = duration;
  this->offset = offset;
  this->fdmaChannel = fdmaChannel;
  this->lower = lower;
  this->status = 1;
}

double GlobalScheduler::NodeTimer::getDuration()
{
  return (duration);
}

double GlobalScheduler::NodeTimer::getoffset()
{
  return offset;
}

int GlobalScheduler::NodeTimer::getfdmaChannel()
{
    return fdmaChannel;
}

int GlobalScheduler::NodeTimer::getstatus()
{
  return status;
}
 
void GlobalScheduler::NodeTimer::setstatus(int var)
{
  this->status = var;
}

glue::convergence::Lower2OFDMAPhy* GlobalScheduler::NodeTimer::getLower2OFDMAAddress()
{
  return lower;
}

GlobalScheduler::NodeTimer::~NodeTimer()
{

}
