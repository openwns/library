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

#include <WNS/probe/bus/ProbeBus.hpp>
#include <WNS/probe/bus/ProbeBusRegistry.hpp>
#include <WNS/simulator/ISimulator.hpp>

#include <iostream>

using namespace wns::probe::bus;

void
wns::probe::bus::addProbeBusses(const wns::pyconfig::View& pyco)
{
    wns::simulator::Registry* reg = wns::simulator::getRegistry();
    assure(reg != NULL, "wns::simulator::Registry is NULL!");

    ProbeBusRegistry* pbreg = reg->find<ProbeBusRegistry*>("WNS.ProbeBusRegistry");
    assure(pbreg != NULL, "Cannot find the ProbeBusRegistry!");

    for(int ii=0 ; ii < pyco.len("subtrees"); ++ii)
    {
        wns::pyconfig::View subpyco = pyco.get("subtrees",ii);
        ProbeBus* pb = pbreg->getProbeBus(subpyco.get<std::string>("probeBusID"));
        for(int jj=0 ; jj < subpyco.len("top"); ++jj)
        {
            pb->addReceivers(subpyco.get("top",jj));
        }
    }
}

void
ProbeBus::addReceivers(const wns::pyconfig::View& pyco)
{
    std::string nameInFactory = pyco.get<std::string>("nameInFactory");
    wns::probe::bus::ProbeBusCreator* c =
        wns::probe::bus::ProbeBusFactory::creator(nameInFactory);
    wns::probe::bus::ProbeBus* pb = c->create(pyco);

    pb->startReceiving(this);

    for(int ii=0; ii < pyco.len("observers"); ++ii)
    {
        pb->addReceivers(pyco.get<wns::pyconfig::View>("observers", ii));
    }
}

void
ProbeBus::forwardMeasurement(const wns::simulator::Time& timestamp,
                             const double& aValue,
                             const IContext& theRegistry)
{
    if (this->accepts(timestamp, theRegistry))
    {
        this->onMeasurement(timestamp, aValue, theRegistry);

        this->forEachObserverNoDetachAllowed(
            ProbeBusMeasurementFunctor(
                &ProbeBusNotificationInterface::forwardMeasurement,
                timestamp,
                aValue,
                theRegistry)
            );
    }
}

void
ProbeBus::forwardOutput()
{
    this->output();

    this->sendNotifies(&ProbeBusNotificationInterface::forwardOutput);
}

void
ProbeBus::startReceiving(ProbeBus* other)
{
    this->startObserving(other);
}

void
ProbeBus::stopReceiving(ProbeBus* other)
{
    this->stopObserving(other);
}
