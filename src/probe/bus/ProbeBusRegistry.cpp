/******************************************************************************
 * WNS (Wireless Network Simulator)                                           *
 * __________________________________________________________________________ *
 *                                                                            *
 * Copyright (C) 2004-2006                                                    *
 * Chair of Communication Networks (ComNets)                                  *
 * Kopernikusstr. 16, D-52074 Aachen, Germany                                 *
 * phone: ++49-241-80-27910 (phone), fax: ++49-241-80-22242                   *
 * email: wns@comnets.rwth-aachen.de                                          *
 * www: http://wns.comnets.rwth-aachen.de                                     *
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
        wns::probe::bus::ProbeBusCreator* c = wns::probe::bus::ProbeBusFactory::creator(protoConf.get<std::string>("nameInFactory"));
        wns::probe::bus::ProbeBus* pb = c->create(protoConf);

        registry.insert(probeBusID, pb);

        return registry.find(probeBusID);
    }
}

void
ProbeBusRegistry::forwardOutput()
{
    for (ProbeBusRegistryContainer::const_iterator it = registry.begin(); it != registry.end(); ++it)
    {
        it->second->forwardOutput();
    }
}
