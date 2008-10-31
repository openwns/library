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

#include <WNS/queuingsystem/MM1Step5.hpp>

#include <WNS/probe/bus/ProbeBusRegistry.hpp>

using namespace wns::queuingsystem;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    SimpleMM1Step5,
    wns::simulator::ISimulationModel,
    "openwns.queuingsystem.SimpleMM1Step5",
    wns::PyConfigViewCreator);

SimpleMM1Step5::SimpleMM1Step5(const wns::pyconfig::View& config) :
    config_(config),
    logger_(config.get("logger")),
    probeBus_(NULL)
{
    wns::pyconfig::View disConfig = config.get("jobInterArrivalTimeDistribution");
    std::string disName = disConfig.get<std::string>("__plugin__");
    jobInterarrivalTime_ = 
        wns::distribution::DistributionFactory::creator(disName)->create(disConfig);

    disConfig = config.get("jobProcessingTimeDistribution");
    disName = disConfig.get<std::string>("__plugin__");
    jobProcessingTime_ = 
        wns::distribution::DistributionFactory::creator(disName)->create(disConfig);
}

// begin example "wns.queuingsystem.mm1step5.doStartup.example" 
void
SimpleMM1Step5::doStartup()
{
    MESSAGE_SINGLE(NORMAL, logger_, "MM1Step5 started, generating first job\n" << *this);

    std::string probeBusName = config_.get<std::string>("probeBusName");

    wns::probe::bus::ProbeBusRegistry* reg = wns::simulator::getProbeBusRegistry();

    probeBus_ = reg->getMeasurementSource(probeBusName);

    // We need that probe bus!!
    assure(probeBus_ != NULL, "ProbeBus could not be created");

    generateNewJob();
}
// end example

void
SimpleMM1Step5::doShutdown()
{
    assure(probeBus_ != NULL, "No ProbeBus");
    probeBus_->forwardOutput();
}

void
SimpleMM1Step5::generateNewJob()
{
    // Create a new job
    Job job = Job();

    queue_.push_back(job);

    wns::simulator::Time delayToNextJob = (*jobInterarrivalTime_)();

    MESSAGE_SINGLE(NORMAL, logger_, "Generated new job, next job in " << delayToNextJob << "s\n" << *this);

    // The job is the only job in the system. There is no job that is currently
    // being served -> the server is free and can process the next job
    if (queue_.size() == 1)
    {
        processNextJob();
    }

    wns::simulator::getEventScheduler()->scheduleDelay(
        boost::bind(&SimpleMM1Step5::generateNewJob, this),
        delayToNextJob);

}

void
SimpleMM1Step5::onJobProcessed()
{
    Job finished = queue_.front();

    queue_.pop_front();

    // Calculate the Jobs sojourn time
    wns::simulator::Time  now;
    wns::simulator::Time  sojournTime;
    now = wns::simulator::getEventScheduler()->getTime();
    sojournTime = now - finished.getCreationTime();

    // Give some debug output
    MESSAGE_SINGLE(NORMAL, logger_, "Finished a job\n" << *this);
    MESSAGE_BEGIN(NORMAL, logger_, m, "Sojourn Time of last job ");
    m << sojournTime;
    MESSAGE_END();

    // Send the measurement to the probeBus
    // We will soon learn what the context of a measurement is. For
    // now, we just create an empty Context object
    wns::probe::bus::Context emptyContext;

    // Forward the measurement onto the probeBus_
    assure(probeBus_ != NULL, "No ProbeBus");
    probeBus_->forwardMeasurement(now, sojournTime, emptyContext);

    // if there are still jobs, serve them
    if (!queue_.empty())
    {
        processNextJob();
    }
}

void
SimpleMM1Step5::processNextJob()
{
    wns::simulator::Time processingTime = (*jobProcessingTime_)();

    wns::simulator::getEventScheduler()->scheduleDelay(
        boost::bind(&SimpleMM1Step5::onJobProcessed, this),
        processingTime);
    MESSAGE_SINGLE(NORMAL, logger_, "Processing next job, processing time: " << processingTime << "s\n" << *this);
}

std::string
SimpleMM1Step5::doToString() const
{
    std::stringstream ss;
    ss << "Jobs in system: " << queue_.size();
    return ss.str();
}
