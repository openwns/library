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

#include <WNS/probe/bus/tests/ProbeBusStub.hpp>

using namespace wns::probe::bus::tests;

ProbeBusStub::ProbeBusStub()
{
    receivedCounter = 0;
    providerName = "";
    filter = 0;
}

ProbeBusStub::~ProbeBusStub()
{
}

void
ProbeBusStub::onMeasurement(const wns::simulator::Time& timestamp,
                            const double& value,
                            const IContext&)
{
    ++receivedCounter;
    receivedTimestamps.push_back(timestamp);
    receivedValues.push_back(value);
}


void
ProbeBusStub::output()
{
}

bool
ProbeBusStub::accepts(const wns::simulator::Time& /*timestamp*/,
                      const IContext& reg)
{
    if(providerName == "")
    {
        return true;
    }

    if(reg.getInt(providerName) == filter)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void
ProbeBusStub::setFilter(std::string _providerName, int32_t _filter)
{
    providerName = _providerName;
    filter = _filter;
}
