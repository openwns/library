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

#include <WNS/simulator/ISimulator.hpp>
#include <WNS/events/scheduler/Interface.hpp>

#include <WNS/probe/bus/SettlingTimeGuard.hpp>

using namespace wns::probe::bus;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    SettlingTimeGuard,
    wns::probe::bus::ProbeBus,
    "SettlingTimeGuard",
    wns::PyConfigViewCreator);

SettlingTimeGuard::SettlingTimeGuard(const wns::pyconfig::View&):
	settlingTime_(wns::simulator::getConfiguration().get<double>("WNS.PDataBase.settlingTime"))
{
}

SettlingTimeGuard::~SettlingTimeGuard()
{
}

bool
SettlingTimeGuard::accepts(const wns::simulator::Time&, const IContext&)
{
    return wns::simulator::getEventScheduler()->getTime() >= settlingTime_;
}

void
SettlingTimeGuard::onMeasurement(const wns::simulator::Time&,
								 const double&,
								 const IContext&)
{
}

void
SettlingTimeGuard::output()
{
}
