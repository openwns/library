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

#include <WNS/scheduler/metascheduler/NoMetaScheduler.hpp>
#include <WNS/scheduler/metascheduler/MetaScheduler.hpp>
#include <WNS/scheduler/strategy/StrategyInterface.hpp>
#include <WNS/scheduler/SchedulerTypes.hpp>
#include <WNS/scheduler/strategy/Strategy.hpp>

#include <boost/algorithm/string.hpp>

#include <WIMAC/scheduler/Scheduler.hpp>


using namespace wimac::scheduler;
using namespace wns::scheduler::strategy;
using namespace wns::scheduler::metascheduler;


STATIC_FACTORY_REGISTER_WITH_CREATOR(NoMetaScheduler,
                                     IMetaScheduler,
                                     "NoMetaScheduler",
                                     wns::PyConfigViewCreator);


				     
NoMetaScheduler::NoMetaScheduler(const wns::pyconfig::View& _config):MetaScheduler(_config)
{
}


void NoMetaScheduler::optimize(void)
{

}

void NoMetaScheduler::doOptimize(void)
{
  return;
}


/*
void NoMetaScheduler::provideMetaConfiguration(const wns::scheduler::strategy::StrategyInput* strategyInput, wns::scheduler::SchedulingMapPtr schedulingMap)
{
 
 return;

}
*/



