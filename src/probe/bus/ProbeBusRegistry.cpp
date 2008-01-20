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
    protoConf(pyco.getView("prototype")),
    registry()
{
}

ProbeBusRegistry::~ProbeBusRegistry()
{
}

ProbeBus*
ProbeBusRegistry::getProbeBus(const std::string& probeBusID)
{
    if (registry.knows(probeBusID))
    {
        return registry.find(probeBusID);
    }
    else
    {
        std::string name = protoConf.get<std::string>("nameInFactory");
        wns::probe::bus::ProbeBusCreator* c =
            wns::probe::bus::ProbeBusFactory::creator(name);

        wns::probe::bus::ProbeBus* pb = c->create(protoConf);

        registry.insert(probeBusID, pb);

        return registry.find(probeBusID);
    }
}

void
ProbeBusRegistry::forwardOutput()
{
    for (ProbeBusRegistryContainer::const_iterator it = registry.begin();
         it != registry.end();
         ++it)
    {
        it->second->forwardOutput();
    }
}
