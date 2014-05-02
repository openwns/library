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
#include "WNS/scheduler/metascheduler/TimeoutHelper.hpp"

// TimeoutHelper
TimeoutHelper::TimeoutHelper(glue::convergence::Lower2OFDMAPhy* motherClass)
{

    status = 0;
    time = 0;
    this->motherClass = motherClass;
    
}

void TimeoutHelper::onTimeout()
{

    if (status == DEAKTIVATETIME)
    {
        motherClass->deactivateSlot();
    }
    
    if (status == ONSLOTSTART)
    {
        motherClass->onSlotStart();
    }
    
}

void TimeoutHelper::timerCall(int status, double time)
{

    this->status = status;
    this->time = time;
    
    setNewTimeout(time/1000.0);
}

FlightManager::FlightManager(glue::convergence::Lower2OFDMAPhy* motherClass, double lamda, double mue)
{
    this->motherClass = motherClass;
    this->lamda = lamda;
    this->mue = mue;

#ifdef ENABLE_ARRIVAL_OF_PLANES
    arrival = new MMnArrival(this,lamda);
    process = new MMnProcess(this,mue);

    arrival->MMnStart();
#else
    GlobalScheduler::getInstance().addnode(motherClass);
#endif
}

void FlightManager::onArrival()
{
    static double erfolg;
    static double misserfolg;
    // tell globalscheduler
    if (GlobalScheduler::getInstance().addnode(motherClass)) {
        erfolg++;
        process->MMnStart();
    }
    else {
        misserfolg++;
        GlobalScheduler::getInstance().deletenode(motherClass);
        arrival->MMnStart();
    }
    wns::simulator::Time now = wns::simulator::getEventScheduler()->getTime();


}

void FlightManager::onProcess()
{
    // tell globalscheduler
    if (GlobalScheduler::getInstance().deletenode(motherClass) == false) {
    }

    arrival->MMnStart();
}


MMnArrival::MMnArrival(FlightManager* flightManager, double lamda)
{
    myFlightManager = flightManager;
    this->lamda = lamda;

}

void MMnArrival::MMnStart()
{
    static wns::distribution::NegExp interArrivalDist = wns::distribution::NegExp(1/lamda);
    double iat = interArrivalDist();

   
    setTimeout(iat);
}


void MMnArrival::onTimeout()
{
    myFlightManager->onArrival();
}

MMnArrival::~MMnArrival()
{
    
}


MMnProcess::MMnProcess(FlightManager* flightManager, double mue)
{
    myFlightManager = flightManager;
    this->mue = mue;

}

void MMnProcess::MMnStart()
{
    static wns::distribution::NegExp processingTimeDist = wns::distribution::NegExp(1/mue);
    double prt = processingTimeDist();

    setTimeout(prt);
}


void MMnProcess::onTimeout()
{
    myFlightManager->onProcess();

}

MMnProcess::~MMnProcess()
{
    
}

