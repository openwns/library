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
#include <algorithm>
#include <numeric>
#include <limits.h>
#include <math.h>
#include <sys/time.h> 


#include <WNS/scheduler/metascheduler/MaxRegretMetaScheduler.hpp>
#include <WNS/scheduler/metascheduler/MetaScheduler.hpp>
#include <WNS/scheduler/strategy/StrategyInterface.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>
#include <WNS/scheduler/strategy/Strategy.hpp>

#include <boost/algorithm/string.hpp>

#include <WIMAC/scheduler/Scheduler.hpp>
#include <sys/times.h>
#include <time.h>

using namespace wimac::scheduler;
using namespace wns::scheduler::strategy;
using namespace wns::scheduler::metascheduler;


STATIC_FACTORY_REGISTER_WITH_CREATOR(MaxRegretMetaScheduler,
                                     IMetaScheduler,
                                     "MaxRegretMetaScheduler",
                                     wns::PyConfigViewCreator);




MaxRegretMetaScheduler::MaxRegretMetaScheduler(const wns::pyconfig::View& _config):MetaScheduler(_config)
{

}

struct WeightTuple
{
  WeightTuple (double weight, std::vector<int> pos) {_weight = weight; _pos = pos;}
  
  double _weight;
  std::vector<int> _pos;
  
  bool operator < (const WeightTuple& wt) const
  {
    if (_weight < wt._weight)
      return true;
    else if (_weight > wt._weight)
      return false;
    else
    {
      for (int i=0; i < _pos.size(); i++)
      {
	if (_pos[i] < wt._pos[i])
	  return true;
	else if (_pos[i] > wt._pos[i])
	  return false;
	else
	  continue;
      }
    }
  }
};
  
void MaxRegretMetaScheduler::optimize(const UtilityMatrix& throughputMatrix, std::vector< std::vector<int> >& vBestCombinations)
{
  
  // matrix parameter
  int iBaseStations = throughputMatrix.getDimensions().first;
  int iNumberUTperBS = throughputMatrix.getDimensions().second[0];
  int iMatrixSize = throughputMatrix.getMatrixSize();

  
  // variables
  double dAccumValue = 0;  
  double bestDataRate = 0.0;
  static bool bComputed = false;
  
  std::vector<int> vBaseStationsCounter;
  std::vector<int> vBaseStationsSize;
  std::vector< std::vector<bool> > vValidIndices (iBaseStations);
  std::vector< std::vector<double> > MaxValues (iBaseStations); 
  
  // Setup data
  for (int i = 0; i < iBaseStations; i++)
  {
    std::vector<int> vCombination;
    int iSize = baseStations[i]->vActiveUsers.size();
    
    vBaseStationsSize.push_back(iSize);
    vValidIndices[i].resize (vBaseStationsSize[i], true);
    MaxValues[i].resize (vBaseStationsSize[i], 0.0);
    iMatrixSize *= iSize;
  }
  
  vBaseStationsCounter.clear();
  vBaseStationsCounter.resize(iBaseStations, 0);
  
    
 

  //Max Regret Greedy
  for (int b=0; b < vBaseStationsSize[0]; ++b)
  {
    std::vector<int> vCurrentBest (iBaseStations, 0);
    double currentBestRegret = -1.0;
    double currentDataRate = 0.0;
    
  
    for (int b2=0; b2 < iBaseStations; ++b2)
    {
      
      for (int ut=0; ut < vBaseStationsSize[0]; ++ut)
      {
	//std::cout << "BS: " << b2 << " UT: " << ut << "\n";
	std::set<WeightTuple> twoBestVectors;
	
	if (!vValidIndices[b2][ut])
	{
	  //std::cout << "blocked\n";
	  continue;
	}
	//Seach for two biggest vectors
	vBaseStationsCounter.clear();
	vBaseStationsCounter.resize(iBaseStations, 0);
	vBaseStationsCounter[b2] = ut;
	
	for (int i=0; i < iMatrixSize/vBaseStationsSize[b2]; ++i)
	{
	  //std::cout << "InsideLoop" << "\n";
	  //Walk over matrix
	  for (int j=0; j < iBaseStations; ++j)
	  {
	    if (j == b2)
	      continue;
	    
	    vBaseStationsCounter[j]++;
	    if (vBaseStationsCounter[j] == vBaseStationsSize[j])
	    {
	      vBaseStationsCounter[j] = 0;
	      continue;
	    }
	    else
	      break;
	  }
	  
	  //std::cout << "Counter: ";
	  /*
	  for (int g=0; g < vBaseStationsCounter.size(); g++)
	  {
	    std::cout << vBaseStationsCounter[g] << ", ";
	  }
	  */
	  
	  //Test for a valid vector
	  bool bBlocked = false;
	  for (int j=0; j < iBaseStations; ++j)
	  {
	    if (!vValidIndices[j][vBaseStationsCounter[j]])
	    {
	      bBlocked = true;
	      break;
	    }
	  }
	  if (bBlocked)
	  {
	    //std::cout << "Blocked: ";
	    for (int g=0; g < vBaseStationsCounter.size(); g++)
	    {
	      //std::cout << vBaseStationsCounter[g] << ", ";
	    }
	    //std::cout << "\n";
	    continue;
	  }
	  
	  //Get Value
	  double dValue = throughputMatrix.getValue(vBaseStationsCounter);
	  
	  //std::cout << " Value: " << dValue << "\n";
	  twoBestVectors.insert(WeightTuple(dValue, vBaseStationsCounter));
	  //std::cout << "CurrentValue:" << dValue << " big: " << twoBestVectors.rbegin()->_weight << "\n";
	  if (twoBestVectors.size() > 2)
	  {
	    twoBestVectors.erase(twoBestVectors.begin());
	    //std::cout << "NewBig: " << twoBestVectors.rbegin()->_weight << "\n";
	    //std::cout << "NewSmall: " << twoBestVectors.begin()->_weight << "\n";
	  }
	}
	
	//std::cout << "Size: " << twoBestVectors.size() << "\n";
	if (twoBestVectors.size() == 0)
	{
	  continue;
	}
	else if (twoBestVectors.size() == 1)
	{
	  //std::cout << "CurrentBestRegret: " << currentBestRegret << "\n";
	  if (0.0 > currentBestRegret)
	  {
	    vCurrentBest = twoBestVectors.rbegin()->_pos;
	    currentBestRegret = 0.0;
	    currentDataRate = twoBestVectors.rbegin()->_weight;
	    //std::cout << "CurrentBestRegret: " << 0.0 << " Weight: " << currentDataRate << " Pos: ";
	    for (int g=0; g < vCurrentBest.size(); g++)
	    {
	     // std::cout << vCurrentBest[g] << ", ";
	    }
	    //std::cout << "\n";
	  }
	}
	else if (twoBestVectors.size() == 2)
	{
	  double regret = (twoBestVectors.rbegin()->_weight - twoBestVectors.begin()->_weight);
	
	  if (regret > currentBestRegret)
	  {
	    vCurrentBest = twoBestVectors.rbegin()->_pos;
	    currentBestRegret = regret;
	    currentDataRate = twoBestVectors.rbegin()->_weight;
	    //std::cout << "CurrentBestRegret: " << regret << " Weight: " << currentDataRate << " Pos: ";
	    for (int g=0; g < vCurrentBest.size(); g++)
	    {
	      //std::cout << vCurrentBest[g] << ", ";
	    }
	    //std::cout << "\n";
	  }
	}
	
	//compare two biggest vectors with previous result
	//std::set<WeightTuple>::iterator it = twoBestVectors.begin()++;
	

      }
    }
    
    
    
    //std::cout << "FinalBestRegret: " << currentBestRegret << " DataRate: " << currentDataRate << " Pos: ";
    dAccumValue += currentDataRate;
    
    //Apply greedy result to best Combination vectors
    for (int j=0; j < iBaseStations; ++j)
    {
      //std::cout << vCurrentBest[j] << ", ";
      baseStations[j]->bestCombination[b] = vCurrentBest[j];
      (vBestCombinations)[j][b] = vCurrentBest[j];
      vValidIndices[j][vCurrentBest[j]] = false;
    }
    //std::cout << std::endl;
    
    for (int j=0; j < iBaseStations; ++j)
    {
      for (int k=0; k < vBaseStationsSize[0]; ++k)
      {
	//std::cout << ValidIndices[j][k] << " "; 
      }
      //std::cout << std::endl;
    } 
  }
  

  /*
  std::cout<<"MaxRegret"<<std::endl;
  
  for (int b=0; b < iBaseStations; ++b)
  {
    std::cout  << "BS: " << baseStations[b]->BSID.getName() << " ";
    for (int i=0; i < baseStations[b]->bestCombination.size(); ++i)
    {
      std::cout  << baseStations[b]->bestCombination[i] << ", ";
    }
    std::cout << std::endl;
  }
  */
  
}




