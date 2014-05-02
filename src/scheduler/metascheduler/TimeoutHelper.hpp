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
#ifndef WNS_SCHEDULER_METASCHEDULER_TIMEOUTHELPER_HPP
#define WNS_SCHEDULER_METASCHEDULER_TIMEOUTHELPER_HPP

#include <GLUE/convergence/Lower2OFDMAPhy.hpp>
#include <GLUE/convergence/Upper.hpp>

#include <WNS/pyconfig/View.hpp>
#include <WNS/events/CanTimeout.hpp>

#include <WNS/distribution/NegExp.hpp>
#include <WNS/distribution/Poisson.hpp>

#include "GlobalScheduler.hpp"
// Makros

#define ONSLOTSTART 1
#define DEAKTIVATETIME 2

class MMnArrival;
class MMnProcess;


/******** Timeout helper class for Lower2OFDMAPhy **************
*  Manage the timeouts of each TDMA FDMA slot for each aircraft
****************************************************************/
class TimeoutHelper:
public wns::events::CanTimeout
{
    public:
        TimeoutHelper(glue::convergence::Lower2OFDMAPhy* motherClass);
        //~TimeoutHelper();

        /**
        * @brief calls the time. regards status variable.
        */
        void timerCall(int status, double time);
    private:
        glue::convergence::Lower2OFDMAPhy* motherClass;
        virtual void onTimeout();
        int status;
        double time;
};

/******** FlightManager ****************************************
*  Manage activ aircraft
*  Arrival and process of each aircraft
****************************************************************/

class FlightManager
{
    public:
        
        FlightManager(glue::convergence::Lower2OFDMAPhy* motherClass, double lamda, double mue);

        /**
        * @brief call this function when aircraft gets aktiv.
        */
        void onArrival();
        /**
        * @brief call this function when aircraft gets inaktiv.
        */
        void onProcess();
        
    private:
        
        glue::convergence::Lower2OFDMAPhy* motherClass;
        double mue;
        double lamda;
        MMnArrival* arrival;
        MMnProcess* process;
};

/******** MMnArrival ****************************************
*   random generator for arrival of aircraft
****************************************************************/
class MMnArrival :
public wns::events::CanTimeout
    //public wns::simulator::ISimulationModel
    {
        public:
            explicit
            MMnArrival(FlightManager* flightManager,double lamda);
            
            virtual
            ~MMnArrival();

            void MMnStart();
            
        private:
            /**
            * @brief after timeout, this function is called.
            */
            virtual void onTimeout();
            double lamda;
            FlightManager* myFlightManager;
    };


/******** MMnProcess ****************************************
*   random generator for processing aircraft
****************************************************************/
class MMnProcess :
    public wns::events::CanTimeout
    //public wns::simulator::ISimulationModel
{
        public:
            explicit
            MMnProcess(FlightManager* flightManager, double mue);
            
            virtual
            ~MMnProcess();

            void MMnStart();
                
        private:
            FlightManager* myFlightManager;
            double mue;

            /**
            * @brief after timeout, this function is called.
            */
            virtual void onTimeout();
};
        

#endif // WNS_SCHEDULER_METASCHEDULER_TIMEOUTHELPER_HPP