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

// begin example "wns.queuingsystem.mm1step2.hpp.example"
#ifndef WNS_QUEUINGSYSTEM_MM1STEP2_HPP
#define WNS_QUEUINGSYSTEM_MM1STEP2_HPP

#include <WNS/queuingsystem/Job.hpp>

#include <WNS/rng/RNGen.hpp>
#include <WNS/simulator/ISimulator.hpp>
#include <WNS/simulator/ISimulationModel.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/IOutputStreamable.hpp>
#include <WNS/pyconfig/View.hpp>

#include <boost/bind.hpp>


namespace wns { namespace queuingsystem {

    class SimpleMM1Step2 :
        public IOutputStreamable,
        public wns::simulator::ISimulationModel
    {
        typedef wns::rng::VariateGenerator< boost::exponential_distribution<> > Exponential;
    public:
        explicit
        SimpleMM1Step2(const wns::pyconfig::View& configuration);

    private:
        virtual void
        doStartup();

        virtual void
        doShutdown();

        void
        generateNewJob();

        void
        onJobProcessed();

        void
        tryToProcessNextJob();

        void
        processNextJob();

        virtual std::string
        doToString() const;

        Exponential jobInterarrivalTime_;

        Exponential jobProcessingTime_;

        std::list<Job> queue_;

        wns::logger::Logger logger_;
    };
}
}

#endif // NOT defined WNS_QUEUINGSYSTEM_MM1STEP2_HPP
// end example

/**
 * @page wns.queuingsystem.mm1step2 Queuingsystem Tutorial Step 2
 *
 * @section contents Contents
 *   -# @ref queue
 *   -# @ref statefuljobs
 *
 * @section queue Implementing the Queue
 *   - Introduce Job class
 *   - Show how std::list may be used as container
 *   - Show how jobs are queued
 * @include "wns.queuingsystem.Job.hpp.example"
 * @include "wns.queuingsystem.mm1step2.generateNewJob.example"
 *
 * @section statefuljobs Stateful Jobs
 *
 *   - Now we want to create some statistic for our queuing system, e.g. we want
 *   to compute the sojourn time of a job. Therefore the job has to remember its
 *   time when it has been created. In the constructor of the Job class we get
 *   the current simulation time, by requesting the current simulation time from
 *   the EventScheduler of the simulator.
 *   - When the job has been processed we can
 *   calculate the sojourn time, which is the time a job stayed in the
 *   queue. Therefore we take the actual time - again with the
 *   assistance of the EventScheduler - and subtract the
 *   creation time of the job. The first code snippet shows the Job class and
 *   the getter method (getCreationTime()) that is needed in order to retrieve
 *   the creation time of the finished job. The second code snippet gives an
 *   example for the implmentation of the debug output of the sojourn
 *   time. Whenever the queue processes a job, it takes the oldest job of the
 *   queue (first-in-first-out strategy) by calling the front method. Afterwards
 *   the job is removed from the queue by calling the pop method. Now we use the
 *   logger to output the sojourn time. Afterwards the queue continues to
 *   process the remaining jobs, if any.
 * @include "wns.queuingsystem.Job.cpp.example"
 * @include "wns.queuingsystem.mm1step2.onJobProcessed.example"
 */

