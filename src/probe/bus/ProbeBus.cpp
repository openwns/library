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

ProbeBus::ProbeBus():
    observer(this)
{
}

ProbeBus::~ProbeBus()
{
}

void
ProbeBus::forwardMeasurement(const wns::simulator::Time& timestamp,
                             const double& aValue,
                             const IContext& theRegistry)
{
    if (this->accepts(timestamp, theRegistry))
    {
        this->onMeasurement(timestamp, aValue, theRegistry);

        subject.forwardMeasurement(timestamp, aValue, theRegistry);
    }
}

void
ProbeBus::forwardOutput()
{
    this->output();

    subject.forwardOutput();
}

void
ProbeBus::startObserving(ProbeBus* other)
{
    observer.startObserving( &(other->subject) );
}

void
ProbeBus::stopObserving(ProbeBus* other)
{
    observer.stopObserving( &(other->subject) );
}
