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

#include <WNS/probe/bus/ProbeBusRegistry.hpp>
#include <WNS/probe/bus/MasterProbeBus.hpp>

using namespace wns::probe::bus;

ProbeBusRegistry::ProbeBusRegistry(const wns::pyconfig::View& pyco):
    pyco_(pyco),
    registry_()
{
}

ProbeBusRegistry::~ProbeBusRegistry()
{
}

void
ProbeBusRegistry::startup()
{
    this->connectProbeBusses(pyco_);
}

void
ProbeBusRegistry::connectProbeBusses(const wns::pyconfig::View& probeBusTrees)
{
    for(int ii=0 ; ii < probeBusTrees.len("subtrees"); ++ii)
    {
        wns::pyconfig::View subpyco = probeBusTrees.get("subtrees",ii);
        ProbeBus* pb = this->getProbeBus(subpyco.get<std::string>("probeBusID"));
        for(int jj=0 ; jj < subpyco.len("top"); ++jj)
        {
            pb->addReceivers(subpyco.get("top",jj));
        }
    }
}

ProbeBus*
ProbeBusRegistry::getProbeBus(const std::string& probeBusID)
{
    if (!registry_.knows(probeBusID))
    {
        wns::pyconfig::View protoConf = pyco_.get("prototype");
        std::string name = protoConf.get<std::string>("nameInFactory");
        wns::probe::bus::ProbeBusCreator* c =
            wns::probe::bus::ProbeBusFactory::creator(name);

        wns::probe::bus::ProbeBus* pb = c->create(protoConf);

        registry_.insert(probeBusID, pb);
    }
    return registry_.find(probeBusID);
}

void
ProbeBusRegistry::forwardOutput()
{
    for (ProbeBusRegistryContainer::const_iterator it = registry_.begin();
         it != registry_.end();
         ++it)
    {
        it->second->forwardOutput();
    }
}
