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

#include <WNS/probe/bus/MasterProbeBus.hpp>

using namespace wns::probe::bus;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    MasterProbeBus,
    wns::probe::bus::ProbeBus,
    "MasterProbeBus",
    wns::PyConfigViewCreator);

MasterProbeBus::MasterProbeBus(const wns::pyconfig::View&)
{
}

bool
MasterProbeBus::accepts(const wns::simulator::Time&, const IContext&)
{
    // We always accept everything
    return true;
}

void
MasterProbeBus::onMeasurement(const wns::simulator::Time&,
                              const double&,
                              const IContext&)
{
    // We do not do anything with a measurement
    // ProbeBus will forward to all attached servers
}

void
MasterProbeBus::output()
{
    // Nothing needs to be done here
}
