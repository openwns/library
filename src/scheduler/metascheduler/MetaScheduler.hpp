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

#ifndef WNS_SCHEDULER_METASCHEDULER_METASCHEDULER_HPP
#define WNS_SCHEDULER_METASCHEDULER_METASCHEDULER_HPP

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iostream>

#include <WNS/container/Registry.hpp>
#include <WNS/scheduler/strategy/StrategyInterface.hpp>
#include <WNS/StaticFactory.hpp>
#include <WNS/scheduler/metascheduler/IMetaScheduler.hpp>
#include <WNS/Singleton.hpp>
#include <WNS/scheduler/strategy/Strategy.hpp>



//#include <WIMAC/scheduler/Scheduler.hpp>



//using namespace wimac::scheduler;


namespace wns { namespace scheduler{ namespace metascheduler{
	
	class InterferenceMatrix {
	  public:
	    void createMatrix (int baseStations, std::vector<int>& userTerminalsInBaseStations);
	    void setValue (std::vector<int>& userIndices, double value);
	    double getValue (std::vector<int>& userIndices);
	    
	    void Print (void);
	    //friend std::ostream& operator<<(std::ostream &stream, const InterferenceMatrix& m);
	    
	  private:
	    int _baseStations;
	    std::vector<int> _userTerminalsInBaseStations;
	    std::vector<int> _indexJumpOfBaseStation;
	    std::vector<double> _data;
	};
	
	/*std::ostream& operator<<(std::ostream &stream, const InterferenceMatrix& m)
	{
	  //We only print the first two dim
	  if (m._baseStations < 2)
	    return stream;
	  
	  for (int y=0; y < m._userTerminalsInBaseStations[1]; ++y)
	  {
	    for (int x=0; x < m._userTerminalsInBaseStations[0]; ++x)
	    {
	      stream << m._data[x + y * m._indexJumpOfBaseStation[1]] << " ";
	    }
	    stream << std::endl;
	  }
	  stream << std::endl;
	  return stream;
	}*/

	class MetaScheduler : public IMetaScheduler
	//, public wns::scheduler::strategy::Strategy 
	      {
	  
	  public:
	    
		MetaScheduler(const wns::pyconfig::View& _config);//: availableFrequencyChannels(0), numberBS(0),numberCount(1) {}						
		
		~MetaScheduler()=0;
				
		unsigned int factorial (unsigned int value);
		 /**
		 * @brief Attach a Scheduler to the MetaScheduler
		 *
		 * Called by the constructor of the Scheduler, hence there is no
		 * need to call this method explictly.
		 */		
		
		virtual void attachBS(const wns::pyconfig::View *pyConfig, 
				      wns::scheduler::RegistryProxyInterface* registryProxy, bool IamUplinkMaster);
		virtual void attachUT(const wns::pyconfig::View *pyConfig, 
				      wns::scheduler::RegistryProxyInterface* registryProxy);
		
		virtual void detach(const std::string &oldScheduler);
		
		
		/**
		 * @brief Returns a StrategyInput.
		 *
		 */						
		virtual wns::scheduler::strategy::StrategyInput 
		*returnStrategyInputBS(wns::scheduler::RegistryProxyInterface* registryProxy,bool IamUplinkMaster);
		virtual wns::scheduler::strategy::StrategyInput *
		returnStrategyInputUT(wns::scheduler::RegistryProxyInterface* registryProxy);
	
		
		
		/**
		 * @brief Modifys a SchedulingMap.
		 *
		 */			
		void provideMetaConfiguration(const wns::scheduler::strategy::StrategyInput* strategyInput, 
						      wns::scheduler::SchedulingMapPtr schedulingMap);
						      
		virtual void optimize(void)=0;
		virtual void doOptimize(void)=0;
		/**
		 * @brief Returns the number of available Frequency Channels for each BS in average.
		 *	  later: if BS have frequency channels in a different frequency range this function can be 
		 *		 used to calculate the overall available frequency channels
		 */	
		//virtual int getAvailableFrequencyChannels(void);
		

		
		/**
		 * @brief Returns the current schedulerName belonging to the StrategyInput
		 *
		 */				
		 bool setCurrentBS(const wns::scheduler::strategy::StrategyInput *strategyInput);
		
		
			
		/**
		 * @brief creates the scheduling pattern for each schedulingMap
		 *
		 */				
		//void startMetaScheduling(std::string currentBS,int frameNr,wns::scheduler::SchedulingMap & schedulingMap, bool isUplink);

				
		
		/**
		 * @brief blocks a Sub-Channel in a schedulingMap
		 *
		 */
		//virtual void blockSubChannel(wns::scheduler::SchedulingMap & schedulingMap,int subChannel);
		//virtual void blockPRB(wns::scheduler::UserID userID,wns::scheduler::SchedulingMap & schedulingMap, int subChannel, int timeSlot, int spatialLayer);
		
		
		
		/**
		 * @brief reserves a Sub-Channel in a schedulingMap
		 *
		 */
		//virtual void reserveSubChannelForUserID(wns::scheduler::SchedulingMap & schedulingMap,wns::scheduler::UserID userID,int subChannel);
		
	
		
		/**
		 * @brief reserves PRBs according to the parameters 
		 *
		 */
		
		//virtual void reservePRB(wns::scheduler::PhysicalResourceBlock *prbDescriptor,wns::scheduler::UserID userID);
		//virtual void reservePRB(wns::scheduler::UserID userID,wns::scheduler::SchedulingMap & schedulingMap, int subChannel, int timeSlot, int spatialLayer);
		//virtual void setPhyModeForPRB(wns::scheduler::UserID userID,wns::scheduler::SchedulingMap & schedulingMap, int subChannel, int timeSlot, int spatialLayer);
		//virtual void setPhyModeForPRB(wns::scheduler::UserID userID, wns::scheduler::SchedulingMap & schedulingMap, int subChannel, int timeSlot, int spatialLayer, wns::service::phy::phymode::PhyModeInterfacePtr pm);
		
		void setActiveUserSet(BSInfo* p_BSInfo, int frameNr);
		void computeRessourceBlockSizes (BSInfo* p_BSInfo);
		//virtual void applyPeerMapping (peerGroup* PeerGroup);
		//virtual void applyPeerMapping (void);
		//virtual void applyFrequencyMap (void);
		//virtual void writeSchedulingToFile (void);
		//virtual void applyBestPermutation (void);
		//virtual void applyBestPermutationBlocks (void);
		//virtual void updateUserSubchannels (void);
		//virtual bool computeInterferenceMap (void);
		/*
		//virtual void GetInterfererOnFrequency (BSInfo* pBS, int iFrequency,
						     std::vector<std::vector<int> >& currentCombination,
						     std::set<wns::scheduler::UserID>& interferer);
		//virtual void GetInterfererOnFrequency (wns::scheduler::UserID user, int iFrequency,
				      std::set<wns::scheduler::UserID>& interferer);
				      
		//virtual double GetUserScore (BSInfo* pBS,
					   wns::scheduler::UserID user,
					   std::set<wns::scheduler::UserID>& interferer);
		//virtual wns::Ratio estimateSINR (BSInfo* pBS,
					   wns::scheduler::UserID user,
					   std::set<wns::scheduler::UserID>& interferer);
		//virtual wns::scheduler::ChannelQualityOnOneSubChannel getMeasurementsForUser (wns::scheduler::UserID);
		*/
		
		
		/**
		 * @brief Shut down all Schedulers (at end of simulation)
		 */
		//void shutDownScheduler();

	        

	};
		  
  }
}


//typedef wns::SingletonHolder<wns::scheduler::metascheduler::MetaScheduler> TheMetaScheduler;

}




#endif // WNS_SCHEDULER_METASCHEDULER_METASCHEDULER_HPP
