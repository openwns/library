/*******************************************************************************
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2004-2007
 * Chair of Communication Networks (ComNets)
 * Kopernikusstr. 16, D-52074 Aachen, Germany
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

#include <WNS/simulator/Simulator.hpp>
#include <WNS/events/scheduler/Interface.hpp>
#include <WNS/logger/Master.hpp>
#include <WNS/rng/RNGen.hpp>
#include <WNS/Assure.hpp>

using namespace wns::simulator;


Simulator::Simulator(const wns::pyconfig::View& configuration) :
    eventScheduler_(NULL),
    masterLogger_(NULL),
    rng_(NULL),
    registry_(new Registry()),
    resetSignal_(new ResetSignal()),
    configuration_(configuration)
{
    this->configureEventScheduler(configuration_.getView("eventScheduler"));
    this->configureMasterLogger(configuration_.getView("masterLogger"));
    this->configureRNG(configuration_.getView("rng"));
}

Simulator::~Simulator()
{
}

wns::events::scheduler::Interface*
Simulator::doGetEventScheduler() const
{
    return eventScheduler_.get();
}

wns::logger::Master*
Simulator::doGetMasterLogger() const
{
    return masterLogger_.get();
}

wns::rng::RNGen*
Simulator::doGetRNG() const
{
    return rng_.get();
}

Registry*
Simulator::doGetRegistry() const
{
    return registry_.get();
}

ResetSignal*
Simulator::doGetResetSignal() const
{
    return resetSignal_.get();
}

wns::pyconfig::View
Simulator::doGetConfiguration() const
{
    return configuration_;
}

void
Simulator::doReset()
{
    throw Exception("Reset is not allowed in normal operation (simulation run)!");
}

void
Simulator::configureEventScheduler(
    const pyconfig::View& eventSchedulerConfiguration)
{
    assure(eventScheduler_.get() == NULL, "EventScheduler already set / configured");

    std::string sched = eventSchedulerConfiguration.get<std::string>("type");
    wns::events::scheduler::Creator* creator = wns::events::scheduler::Factory::creator(sched);
    eventScheduler_.reset(creator->create());
}

void
Simulator::configureMasterLogger(
    const pyconfig::View& masterLoggerConfiguration)
{
    assure(masterLogger_.get() == NULL, "MasterLogger already set / configured");
    masterLogger_.reset(new wns::logger::Master(masterLoggerConfiguration));
}

void
Simulator::configureRNG(
    const pyconfig::View& rngConfiguration)
{
    assure(rng_.get() == NULL, "RNG already set / configured");
	rng_.reset(new wns::rng::RNGen());
    if (rngConfiguration.get<bool>("useRandomSeed"))
    {
        rng_->seed(rngConfiguration.get<uint32_t>("seed"));
    }
}

