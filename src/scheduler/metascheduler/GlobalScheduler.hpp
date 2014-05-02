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

#ifndef WNS_SCHEDULER_METASCHEDULER_GLOBALSCHEDULER_HPP
#define WNS_SCHEDULER_METASCHEDULER_GLOBALSCHEDULER_HPP

#include <GLUE/convergence/Lower.hpp>
#include <WNS/service/phy/ofdma/DataTransmission.hpp>
#include <WNS/service/phy/ofdma/Notification.hpp>
#include <WNS/service/phy/ofdma/Handler.hpp>
#include <WNS/service/phy/phymode/PhyModeMapperInterface.hpp>

#include <WNS/distribution/NegExp.hpp>

#include "WNS/scheduler/metascheduler/TimeoutHelper.hpp"

#include <GLUE/convergence/Lower2OFDMAPhy.hpp>
#include <GLUE/convergence/Upper.hpp>

#include <WNS/pyconfig/View.hpp>
#include <boost/bind.hpp>

#include "WNS/container/Pool.hpp"
#include "WNS/scheduler/metascheduler/GlobalScheduler.hpp"

#include <WNS/service/dll/Address.hpp>

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iostream>

#include <WNS/container/Registry.hpp>
#include <WNS/StaticFactory.hpp>
#include <WNS/Singleton.hpp>

#include <WNS/pyconfig/View.hpp>
#include <WNS/events/CanTimeout.hpp>

#define MAXNUMBEROFSLOTS 80
#define MAXNUMBEROFACTIVENODES 10000


    /******** Manage the TDMA FDMA slots**********************
    *  save the states of the aircraft
    *  calls the Lower2OFDMAPhy and sets the TDMA FDMA slot
    **********************************************************/
class GlobalScheduler: 
  public wns::events::CanTimeout
{	
      class NodeTimer {
	public:
            /**
            * @brief Subclass to save data for slot position of TDMA FDMA Frame.
            */
	  NodeTimer(double offset,double duration,int fdmaChannel,glue::convergence::Lower2OFDMAPhy* lower);
	  ~NodeTimer();

          /**
          * @brief set the status variable.
          */
	  void setstatus(int var);
          /**
          * @brief get the status variable.
          */
	  int getstatus();
          /**
          * @brief get the TDMA offset to the next slot.
          */
	  double getoffset();
          /**
          * @brief get the FDMA channel.
          */
          int getfdmaChannel();
          /**
          * @brief get the TDMA duration of the next slot.
          */
	  double getDuration();

          
	  glue::convergence::Lower2OFDMAPhy* getLower2OFDMAAddress();
	  
	private:
	  double offset;
	  double duration;
	  int status;
          int fdmaChannel;
	  glue::convergence::Lower2OFDMAPhy* lower;
    };
    
  public:
    
    static GlobalScheduler& getInstance();
    ~GlobalScheduler() {};

    /**
    * @brief add one activ aircraft (node) to the scheduler.
    */
    bool addnode(glue::convergence::Lower2OFDMAPhy* lower);
    
    /**
    * @brief delete one activ aircraft (node) from the scheduler.
    */
    bool deletenode(glue::convergence::Lower2OFDMAPhy* lower);

    /**
    * @brief register one aircraft (node) to the scheduler.
    */
    void registerNode(glue::convergence::Lower2OFDMAPhy* lower);

    void setSlotLength(double slotlength);
    
  private:
    
    NodeTimer* nodeList[MAXNUMBEROFSLOTS];
    glue::convergence::Lower2OFDMAPhy* registerList[MAXNUMBEROFACTIVENODES];
    int numberOfNodes;
    int numberOfActiveNodes;
    int numberOfTDMASlots;
    double periode;
    wns::container::Pool<int> *macPool;
    wns::container::Pool<int> *registerPool;

    /**
    * @brief set the next timeout event.
    */
    void setNextEventTimer();

    /**
    * @brief after timeout, this function is called.
    */
    virtual void onTimeout();

    /**
    * @brief Singleton preparation.
    */
    GlobalScheduler();
    /**
    * @brief Singleton preparation.
    */
    GlobalScheduler( const GlobalScheduler&);
    /**
    * @brief Singleton preparation.
    */
    GlobalScheduler& operator = (const GlobalScheduler &);

    double slotlength_;
};


#endif // WNS_SCHEDULER_METASCHEDULER_GLOBALSCHEDULER_HPP
