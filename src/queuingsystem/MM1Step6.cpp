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

#include <WNS/queuingsystem/MM1Step6.hpp>

#include <WNS/probe/bus/ProbeBusRegistry.hpp>

using namespace wns::queuingsystem;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    SimpleMM1Step6,
    wns::simulator::ISimulationModel,
    "openwns.queuingsystem.SimpleMM1Step6",
    wns::PyConfigViewCreator);

SimpleMM1Step6::SimpleMM1Step6(const wns::pyconfig::View& config) :
    jobInterarrivalTime_(wns::simulator::getRNG(),
                         Exponential::distribution_type(
                             1.0/config.get<wns::simulator::Time>("meanJobInterArrivalTime"))),
    jobProcessingTime_(wns::simulator::getRNG(),
                       Exponential::distribution_type(
                           1.0/config.get<wns::simulator::Time>("meanJobProcessingTime"))),
    priorityDistribution_(wns::simulator::getRNG(),
                          Uniform::distribution_type(Job::lowPriority, Job::highPriority)),
    config_(config),
    logger_(config.get("logger")),
    probeBus_(NULL),
    idle(true)
{
}

// begin example "wns.queuingsystem.mm1step6.doStartup.example" 
void
SimpleMM1Step6::doStartup()
{
    MESSAGE_SINGLE(NORMAL, logger_, "MM1Step6 started, generating first job\n" << *this);

    std::string probeBusName = config_.get<std::string>("probeBusName");

    wns::probe::bus::ProbeBusRegistry* reg = wns::simulator::getProbeBusRegistry();

    probeBus_ = reg->getMeasurementSource(probeBusName);

    // We need that probe bus!!
    assure(probeBus_ != NULL, "ProbeBus could not be created");

    generateNewJob();
}
// end example

void
SimpleMM1Step6::doShutdown()
{
    assure(probeBus_ != NULL, "No ProbeBus");
    probeBus_->forwardOutput();
}

void
SimpleMM1Step6::generateNewJob()
{
    // Create a new job
    Job job = Job(drawJobPriority());

    if (job.getPriority() == Job::lowPriority)
    {
        lowPriorityQueue_.push_back(job);
    }
    else
    {
        highPriorityQueue_.push_back(job);
    }


    wns::simulator::Time delayToNextJob = jobInterarrivalTime_();

    MESSAGE_SINGLE(NORMAL, logger_, "Generated new job, next job in " << delayToNextJob << "s\n" << *this);

    if (idle)
    {
        processNextJob();
    }

    wns::simulator::getEventScheduler()->scheduleDelay(
        boost::bind(&SimpleMM1Step6::generateNewJob, this),
        delayToNextJob);
}

void
SimpleMM1Step6::onJobProcessed()
{
    // Calculate the Jobs sojourn time
    wns::simulator::Time  now;
    wns::simulator::Time  sojournTime;
    now = wns::simulator::getEventScheduler()->getTime();
    sojournTime = now - currentJob_.getCreationTime();

    // Give some debug output
    MESSAGE_SINGLE(NORMAL, logger_, "Finished a job\n" << *this);
    MESSAGE_BEGIN(NORMAL, logger_, m, "Sojourn Time of last job ");
    m << sojournTime;
    MESSAGE_END();

    // Send the measurement to the probeBus
    // We will now create a context which carries information on
    // the jobs priority
    wns::probe::bus::Context context;

    context.insertInt("priority", currentJob_.getPriority());

    // Forward the measurement onto the probeBus_
    assure(probeBus_ != NULL, "No ProbeBus");
    probeBus_->forwardMeasurement(now, sojournTime, context);

    // if there are still jobs, serve them
    if ( getNumberOfJobs() > 0 )
    {
        processNextJob();
    }
    else
    {
        idle = true;
    }
}

void
SimpleMM1Step6::processNextJob()
{
    currentJob_ = getNextJob();

    wns::simulator::Time processingTime = jobProcessingTime_();

    wns::simulator::getEventScheduler()->scheduleDelay(
        boost::bind(&SimpleMM1Step6::onJobProcessed, this),
        processingTime);

    idle = false;

    MESSAGE_SINGLE(NORMAL, logger_, "Processing next job, processing time: " << processingTime << "s\n" << *this);
}

int
SimpleMM1Step6::getNumberOfJobs() const
{
    return lowPriorityQueue_.size() + highPriorityQueue_.size();
}

Job
SimpleMM1Step6::getNextJob()
{
    Job nextJob;

    if (highPriorityQueue_.size() > 0)
    {
        nextJob = highPriorityQueue_.front();

        highPriorityQueue_.pop_front();
    }
    else if(lowPriorityQueue_.size() > 0)
    {
        nextJob = lowPriorityQueue_.front();

        lowPriorityQueue_.pop_front();
    }
    else
    {
        throw wns::Exception("getNextJob called but now Job queued. You must check for available jobs before calling getNextJob!");
    }

    return nextJob;
}

std::string
SimpleMM1Step6::doToString() const
{
    std::stringstream ss;
    ss << "Jobs in system: " << getNumberOfJobs();
    return ss.str();
}

Job::Priority
SimpleMM1Step6::drawJobPriority()
{
    if (priorityDistribution_() == static_cast<int>(Job::lowPriority))
    {
        return Job::lowPriority;
    }
    else
    {
        return Job::highPriority;
    }
}
