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
#include <string>
#include <functional>
#include <iostream>
#include <fstream>
#include <limits.h>

#include <WNS/scheduler/metascheduler/MetaScheduler.hpp>
#include <WNS/scheduler/strategy/StrategyInterface.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>
#include <WNS/scheduler/strategy/Strategy.hpp>

#include <boost/algorithm/string.hpp>

#include <WIMAC/scheduler/Scheduler.hpp>
#include <LTE/timing/RegistryProxy.hpp>

using namespace wimac::scheduler;
using namespace wns::scheduler::strategy;
using namespace wns::scheduler::metascheduler;

/*
STATIC_FACTORY_REGISTER_WITH_CREATOR(MetaScheduler,
                                     IMetaScheduler,
                                     "RandomMetaScheduler",
                                     wns::PyConfigViewCreator);

*/

void InterferenceMatrix::createMatrix (int baseStations, std::vector<int>& userTerminalsInBaseStations)
{
  _baseStations = baseStations;
  _userTerminalsInBaseStations = userTerminalsInBaseStations;
  _indexJumpOfBaseStation.clear();
  _indexJumpOfBaseStation.resize (baseStations);
  _data.clear();
  int numberOfElements = 1;
  for (int i=0; i < _baseStations; ++i)
  {
    _indexJumpOfBaseStation[i] = numberOfElements;
    numberOfElements *= _userTerminalsInBaseStations[i];
    
  }
  _data.resize (numberOfElements);//, wns::Power::from_mW(0.0));
}

void InterferenceMatrix::setValue (std::vector<int>& userIndices, double value)
{
  int index = 0;
  for (int i=0; i < _baseStations; ++i)
  {
    index += _indexJumpOfBaseStation[i] * userIndices[i];
  }
  _data[index] = value;
  
}

double InterferenceMatrix::getValue (std::vector<int>& userIndices)
{
  int index = 0;
  for (int i=0; i < _baseStations; ++i)
  {
    index += _indexJumpOfBaseStation[i] * userIndices[i];
  }
  return _data[index];
}

void InterferenceMatrix::Print (void)
{
  //only for 2 and 3 BS 
  if (_baseStations < 2)
    return;
  
  if (_baseStations == 2)
  {
    for (int y=0; y < _userTerminalsInBaseStations[1]; ++y)
    {
      for (int x=0; x < _userTerminalsInBaseStations[0]; ++x)
      {
	std::cout << _data[x + y * _indexJumpOfBaseStation[1]] << " ";
      }
      std::cout << std::endl;
    }
  }
  else if (_baseStations == 3)
  {
    for (int z=0; z < _userTerminalsInBaseStations[2]; ++z)
    {
      for (int y=0; y < _userTerminalsInBaseStations[1]; ++y)
      {
	for (int x=0; x < _userTerminalsInBaseStations[0]; ++x)
	{
	  std::cout << _data[x + y * _indexJumpOfBaseStation[1] + z * _indexJumpOfBaseStation[2]] << " ";
	}
	std::cout << std::endl;
      }
      std::cout << std::endl;
      std::cout << std::endl;
    }
  }
  
  std::cout << std::endl;
}



MetaScheduler::MetaScheduler(const wns::pyconfig::View& _config) : IMetaScheduler()
{

}

MetaScheduler::~MetaScheduler () {}


void MetaScheduler::attachBS(const wns::pyconfig::View *pyConfig, 
			     wns::scheduler::RegistryProxyInterface* registryProxy,bool IamUplinkMaster)
{
  if(BSMap.find(registryProxy->getMyUserID()) != BSMap.end())
  {
    if(!IamUplinkMaster)
    {
      BSMap.find(registryProxy->getMyUserID())->second->PyConfigDL=pyConfig;
      BSMap.find(registryProxy->getMyUserID())->second->regProxyDL=registryProxy;
    } else {
      BSMap.find(registryProxy->getMyUserID())->second->PyConfigUL=pyConfig;
      BSMap.find(registryProxy->getMyUserID())->second->regProxyUL=registryProxy;
    }
  }else{	    
    if(!IamUplinkMaster)
    {
      BSInfo* tempBS= new BSInfo ;
      tempBS->BSID = registryProxy->getMyUserID();
      tempBS->regProxyDL = registryProxy;
      tempBS->PyConfigDL = pyConfig;
      tempBS->availableFreqChannels = pyConfig->get<int>("freqChannels");
      tempBS->inputDL = NULL;
		
      //0baseStations.push_back (tempBS);
      BSMap.insert(std::make_pair(registryProxy->getMyUserID(), tempBS));
    } else {
      BSInfo* tempBS= new BSInfo ;
      tempBS->BSID = registryProxy->getMyUserID();
      tempBS->regProxyUL = registryProxy;
      tempBS->PyConfigUL = pyConfig;
      tempBS->availableFreqChannels = pyConfig->get<int>("freqChannels");
      tempBS->inputUL = NULL;
			    
      BSMap.insert(std::make_pair(registryProxy->getMyUserID(), tempBS));
      baseStations.push_back (tempBS);
    }			    
  }	  
}

void MetaScheduler::attachUT(const wns::pyconfig::View *pyConfig, wns::scheduler::RegistryProxyInterface* registryProxy)
{
  UTInfo* tempUT= new UTInfo ;
  tempUT->UTID = registryProxy->getMyUserID();
  tempUT->PyConfig = pyConfig;
  tempUT->regProxy = registryProxy;
  tempUT->inputUL = NULL;
		    
  UTMap.insert(std::make_pair(registryProxy->getMyUserID(), tempUT));
}
	
void MetaScheduler::detach(const std::string &oldScheduler)
{
}

StrategyInput *MetaScheduler::returnStrategyInputBS(wns::scheduler::RegistryProxyInterface* registryProxy
	,bool IamUplinkMaster)
{
  
  if(BSMap.find(registryProxy->getMyUserID()) != BSMap.end())
  {
    if (IamUplinkMaster)
    {     
      BSInfo* BS = BSMap.find(registryProxy->getMyUserID())->second;
      StrategyInput *p_strategy= new StrategyInput(BS->PyConfigUL->get<int>("freqChannels"), 
						    BS->PyConfigUL->get<double>("slotDuration"), 
						    BS->PyConfigUL->get<int>("numberOfTimeSlots"), 
						    BS->PyConfigUL->get<int>("maxBeams"),
						    metaScheduler
						    ,NULL);
      BS->inputUL = p_strategy;
      return p_strategy;
    }
    else
    {
      BSInfo* BS = BSMap.find(registryProxy->getMyUserID())->second;
      StrategyInput *p_strategy= new StrategyInput(BS->PyConfigDL->get<int>("freqChannels"), 
						    BS->PyConfigDL->get<double>("slotDuration"), 
						    BS->PyConfigDL->get<int>("numberOfTimeSlots"), 
						    BS->PyConfigDL->get<int>("maxBeams"),
						    metaScheduler
						    ,NULL);
      BS->inputDL = p_strategy; 
      return p_strategy;
    }
  }
  else
  {
    return NULL;
  }
}

StrategyInput *MetaScheduler::returnStrategyInputUT(wns::scheduler::RegistryProxyInterface* registryProxy)
{
  if(UTMap.find(registryProxy->getMyUserID()) != UTMap.end())
  {
    UTInfo* UT = UTMap.find(registryProxy->getMyUserID())->second;
    StrategyInput *p_strategy= new StrategyInput(UT->PyConfig->get<int>("freqChannels"), 
						  UT->PyConfig->get<double>("slotDuration"), 
						  UT->PyConfig->get<int>("numberOfTimeSlots"), 
						  UT->PyConfig->get<int>("maxBeams"),
						  metaScheduler
						  ,NULL);
    UT->inputUL = p_strategy;
    return p_strategy;
  }
  return NULL;
  
}


void MetaScheduler::provideMetaConfiguration(const wns::scheduler::strategy::StrategyInput* strategyInput, wns::scheduler::SchedulingMapPtr schedulingMap)
{
   
  mp_StrategyInput = strategyInput;
  mp_schedulingMap = schedulingMap;
  
  if(!setCurrentBS(strategyInput))
    return;
  
  int frameNr = strategyInput->getFrameNr();
  
  
  //TODO: Separation of MetaScheduler for uplink and downlink
  if(mp_CurrentBSInfo->inputUL == strategyInput)
  {   
    for (int b=0; b < baseStations.size(); b++)
    {     
      setActiveUserSet (baseStations[b], frameNr);
      computeRessourceBlockSizes (baseStations[b]);
    }
    
    bool bBaseStationIsEmpty = false;
    int  iUserCount = BSMap.begin()->second->activeUsers.size();
    bool bAllBaseStationHaveSameSize = true;
    
    for (int b=0; b < baseStations.size(); b++)
    {
      if (baseStations[b]->activeUsers.empty())
	bBaseStationIsEmpty = true;
      
      if (iUserCount != baseStations[b]->activeUsers.size())
	bAllBaseStationHaveSameSize = false;
    }
    
    if (bBaseStationIsEmpty)
    {
      return;
    }
     
    //assure (bAllBaseStationHaveSameSize, "ERROR: Base stations have different UT counts, greedy not applyable!");
    
    
    doOptimize();
  }
  //TODO
  else if (mp_CurrentBSInfo->inputDL == strategyInput)
  {
    mp_StrategyInput = NULL;
    return;
  }
  else
  {
    mp_StrategyInput = NULL;
    return;
  }
  
}

bool MetaScheduler::setCurrentBS(const wns::scheduler::strategy::StrategyInput* strategyInput)
{
  mp_CurrentBSInfo = NULL;
  
  for (int b=0; b < baseStations.size(); ++b)
  {
    if ((baseStations[b]->inputDL==strategyInput)||(baseStations[b]->inputUL==strategyInput))
    {
      mp_CurrentBSInfo = baseStations[b];
      return true;
    } 
  }
  return false;
}


void MetaScheduler::setActiveUserSet(BSInfo* p_BSInfo, int frameNr)
{   
  
  //if set is already present, clear it and recompute
  if (!p_BSInfo->activeUsers.empty())
    p_BSInfo->activeUsers.clear();
  
  if (!p_BSInfo->vActiveUsers.empty())
    p_BSInfo->vActiveUsers.clear();
  
  wns::scheduler::ConnectionSet conns;
  for(int prio = 0; prio < p_BSInfo->regProxyUL->getNumberOfPriorities(); prio++)
  {
      wns::scheduler::ConnectionSet c = p_BSInfo->regProxyUL->getConnectionsForPriority (prio);

      wns::scheduler::ConnectionSet::iterator it;
      for(it = c.begin(); it != c.end(); it++)
	  conns.insert(*it);
  }
  
  wns::scheduler::ConnectionSet::iterator it;

  ////std::cout << "ActiveUsers: ";
  for(it = conns.begin(); it != conns.end(); it++)
  {
    wns::scheduler::UserID user = p_BSInfo->regProxyUL->getUserForCID(*it);
    //Filter out base stations
    if(!user.isBroadcast())
    {
      //Set assumptions does not hold for UserID, so an extra comparision is necessary
      if ( p_BSInfo->activeUsers.find(user) == p_BSInfo->activeUsers.end())
      {
	p_BSInfo->activeUsers.insert(user);
	p_BSInfo->vActiveUsers.push_back(user);
	////std::cout << " " << user.toString();
      }
    }
  }
  ////std::cout << std::endl;
}

void MetaScheduler::computeRessourceBlockSizes (BSInfo* p_BSInfo)
{
  p_BSInfo->resourceBlockSizes.clear();
  int numberOfUEs = p_BSInfo->activeUsers.size();
  if (numberOfUEs == 0)
    return;
  
  int numberSubSchannels = p_BSInfo->availableFreqChannels;
  int numberOfLargeTBs = numberSubSchannels % numberOfUEs;
  int LargeTBSize = floor(numberSubSchannels / numberOfUEs) + 1;
  int numberOfSmallTBs = numberOfUEs - numberOfLargeTBs;
  int SmallTBSize = floor(numberSubSchannels / numberOfUEs);
  
  for (int i=0; i<numberOfLargeTBs; ++i)
  {
    p_BSInfo->resourceBlockSizes.push_back(LargeTBSize);
  }
  for (int i=0; i<numberOfSmallTBs; ++i)
  {
    p_BSInfo->resourceBlockSizes.push_back(SmallTBSize);
  }
}
