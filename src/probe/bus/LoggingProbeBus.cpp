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

#include <WNS/probe/bus/LoggingProbeBus.hpp>

#include <iomanip>

using namespace wns::probe::bus;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    LoggingProbeBus,
    wns::probe::bus::ProbeBus,
    "LoggingProbeBus",
    wns::PyConfigViewCreator);

LoggingProbeBus::LoggingProbeBus(const wns::pyconfig::View& pyco):
    logger(pyco.get<wns::pyconfig::View>("logger"))
{
}

LoggingProbeBus::~LoggingProbeBus()
{
}

bool
LoggingProbeBus::accepts(const wns::simulator::Time&, const IContext&)
{
    return true;
}

void
LoggingProbeBus::onMeasurement(const wns::simulator::Time& timestamp,
                               const double& aValue,
                               const IContext& reg)
{
    MESSAGE_BEGIN(NORMAL, logger, m, "");
    m << "("
      << std::resetiosflags(std::ios::fixed)
      << std::resetiosflags(std::ios::scientific)
      << std::resetiosflags(std::ios::right)
      << std::setiosflags(std::ios::right)
      << std::setiosflags(std::ios::fixed)
      << std::setiosflags(std::ios::dec)
      << std::setprecision(7)
      << std::setw(11)
      << timestamp
      << ") "
      << aValue << " [ "
      << reg << " ]";
    MESSAGE_END();
}

void
LoggingProbeBus::output()
{
}


