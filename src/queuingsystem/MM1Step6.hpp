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

#ifndef WNS_QUEUINGSYSTEM_MM1STEP6_HPP
#define WNS_QUEUINGSYSTEM_MM1STEP6_HPP

#include <WNS/queuingsystem/Job.hpp>

#include <WNS/rng/RNGen.hpp>
#include <WNS/simulator/ISimulator.hpp>
#include <WNS/simulator/ISimulationModel.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/IOutputStreamable.hpp>
#include <WNS/pyconfig/View.hpp>
#include <WNS/probe/bus/ProbeBus.hpp>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

namespace wns { namespace queuingsystem {

    class SimpleMM1Step6 :
        public IOutputStreamable,
        public wns::simulator::ISimulationModel
    {
        typedef wns::rng::VariateGenerator< boost::exponential_distribution<> > Exponential;

        typedef wns::rng::VariateGenerator< boost::uniform_int<> > Uniform;

    public:
        explicit
        SimpleMM1Step6(const wns::pyconfig::View& configuration);

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

        int
        getNumberOfJobs() const;

        Job
        getNextJob();

        virtual std::string
        doToString() const;

        Job::Priority
        drawJobPriority();

        Exponential jobInterarrivalTime_;

        Exponential jobProcessingTime_;

        Uniform priorityDistribution_;

        std::list<Job> lowPriorityQueue_;

        std::list<Job> highPriorityQueue_;

        Job currentJob_;

        wns::pyconfig::View config_;

        wns::logger::Logger logger_;

        wns::probe::bus::ProbeBus* probeBus_;

        bool idle;
    };
}
}

#endif // NOT defined WNS_QUEUINGSYSTEM_MM1STEP6_HPP

/**
 * @page wns.queuingsystem.mm1step6 Queuingsystem Tutorial Step 6
 *
 * @section contents Contents
 *   -# @ref probebusregistry
 *   -# @ref probebustrees
 *
 * @section probebusregistry Seperating Measurement sources and sinks
 *   - Motivate the need of the ProbeBusRegistry by showing scalabilty
 *   problems with the former approach
 *   - Explain the usage of the ProbeBusRegistry both in C++ and Python
 *
 * @include "wns.queuingsystem.mm1step6.doStartup.example"
 *
 * @section probebustrees ProbeBus Trees
 *
 *   - Towards sorting. Show how to add several ProbeBusses to the same
 *   ProbeBusId
 *   - Give an outlook on the usage of Context Information for sorting
 */
