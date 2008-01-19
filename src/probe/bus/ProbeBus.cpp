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

#include <WNS/probe/bus/ProbeBus.hpp>
#include <WNS/probe/bus/ProbeBusRegistry.hpp>
#include <WNS/simulator/ISimulator.hpp>

#include <iostream>

using namespace wns::probe::bus;

void
wns::probe::bus::addProbeBusses(const wns::pyconfig::View& pyco)
{
    ProbeBusRegistry* reg = wns::simulator::getRegistry()->find<ProbeBusRegistry*>("WNS.ProbeBusRegistry");
    for(int ii=0 ; ii < pyco.len("subtrees"); ++ii)
    {
        wns::pyconfig::View subpyco = pyco.get("subtrees",ii);
        ProbeBus* pb = reg->getProbeBus(subpyco.get<std::string>("probeBusID"));
		for(int jj=0 ; jj < subpyco.len("top"); ++jj)
		{
			pb->addReceivers(subpyco.get("top",jj));
		}
    }
}

void
ProbeBus::addReceivers(const wns::pyconfig::View& pyco)
{
    wns::probe::bus::ProbeBusCreator* c = wns::probe::bus::ProbeBusFactory::creator(pyco.get<std::string>("nameInFactory"));
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
