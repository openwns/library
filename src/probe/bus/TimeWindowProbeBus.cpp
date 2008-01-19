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

#include <WNS/probe/bus/TimeWindowProbeBus.hpp>
#include <WNS/container/UntypedRegistry.hpp>


using namespace wns::probe::bus;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    TimeWindowProbeBus,
    wns::probe::bus::ProbeBus,
    "TimeWindowProbeBus",
    wns::PyConfigViewCreator);

TimeWindowProbeBus::TimeWindowProbeBus(const wns::pyconfig::View& pyco):
    evsched(wns::simulator::getEventScheduler()),
    start(pyco.get<wns::simulator::Time>("start")),
    end(pyco.get<wns::simulator::Time>("end"))
{
}

TimeWindowProbeBus::~TimeWindowProbeBus()
{
}

bool
TimeWindowProbeBus::accepts(const wns::simulator::Time&, const IContext&)
{
    return true;
}

void
TimeWindowProbeBus::onMeasurement(const wns::simulator::Time&,
                                  const double&,
                                  const IContext&)
{
}

void
TimeWindowProbeBus::output()
{
}

void
TimeWindowProbeBus::startReceiving(ProbeBus* other)
{
    StartStopReceivingCommand command = StartStopReceivingCommand(this, other, true);

    evsched->schedule(command, start);

    command = StartStopReceivingCommand(this, other, false);

    evsched->schedule(command, end);
}
