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

#include <WNS/queuingsystem/MM1.hpp>

using namespace wns::queuingsystem;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    SimpleMM1,
    wns::simulator::ISimulationModel,
    "openwns.queuingsystem.SimpleMM1",
    wns::PyConfigViewCreator);

SimpleMM1::SimpleMM1(const wns::pyconfig::View& config) :
    jobInterarrivalTime_(wns::simulator::getRNG(),
                         Exponential::distribution_type(
                             1.0/config.get<wns::simulator::Time>("meanJobInterArrivalTime"))),
    jobProcessingTime_(wns::simulator::getRNG(),
                       Exponential::distribution_type(
                           1.0/config.get<wns::simulator::Time>("meanJobProcessingTime"))),
    jobsInSystem_(0),
    logger_(config.get("logger"))
{
}

void
SimpleMM1::doStartup()
{
    MESSAGE_SINGLE(NORMAL, logger_, "MM1 started, generating first job\n" << *this);

    generateNewJob();
}

void
SimpleMM1::doShutdown()
{

}

void
SimpleMM1::generateNewJob()
{
    ++jobsInSystem_;

    wns::simulator::Time delayToNextJob = jobInterarrivalTime_();

    MESSAGE_SINGLE(NORMAL, logger_, "Generated new job, next job in " << delayToNextJob << "s\n" << *this);

    // The job is the only job in the system. There is no job that is currently
    // being served -> the server is free and can process the next job
    if (jobsInSystem_ == 1)
    {
        processNextJob();
    }

    wns::simulator::getEventScheduler()->scheduleDelay(
        boost::bind(&SimpleMM1::generateNewJob, this),
        delayToNextJob);

}

void
SimpleMM1::onJobProcessed()
{
    --jobsInSystem_;
    MESSAGE_SINGLE(NORMAL, logger_, "Finished a job\n" << *this);
    // if there are still jobs, serve them
    if (jobsInSystem_ > 0)
    {
        processNextJob();
    }
}

void
SimpleMM1::processNextJob()
{
    wns::simulator::Time processingTime = jobProcessingTime_();

    wns::simulator::getEventScheduler()->scheduleDelay(
        boost::bind(&SimpleMM1::onJobProcessed, this),
        processingTime);
    MESSAGE_SINGLE(NORMAL, logger_, "Processing next job, processing time: " << processingTime << "s\n" << *this);
}

std::string
SimpleMM1::doToString() const
{
    std::stringstream ss;
    ss << "Jobs in system: " << jobsInSystem_;
    return ss.str();
}
