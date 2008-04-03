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
 *   - Show how to record the time of creation in the Job
 *   - Give debug output for sojourn time after Job finishes
 * @include "wns.queuingsystem.Job.cpp.example"
 * @include "wns.queuingsystem.mm1step2.onJobProcessed.example"
 */

