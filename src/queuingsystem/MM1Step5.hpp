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

#ifndef WNS_QUEUINGSYSTEM_MM1STEP5_HPP
#define WNS_QUEUINGSYSTEM_MM1STEP5_HPP

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

    class SimpleMM1Step5 :
        public IOutputStreamable,
        public wns::simulator::ISimulationModel
    {
        typedef wns::rng::VariateGenerator< boost::exponential_distribution<> > Exponential;
    public:
        explicit
        SimpleMM1Step5(const wns::pyconfig::View& configuration);

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

        wns::pyconfig::View config_;

        wns::logger::Logger logger_;

        wns::probe::bus::ProbeBus* probeBus_;
    };
}
}

#endif // NOT defined WNS_QUEUINGSYSTEM_MM1STEP5_HPP

/**
 * @page wns.queuingsystem.mm1step5 Queuingsystem Tutorial Step 5
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
 * @include "wns.queuingsystem.mm1step5.doStartup.example"
 *
 * @section probebustrees ProbeBus Trees
 *
 *   - Towards sorting. Show how to add several ProbeBusses to the same
 *   ProbeBusId
 *   - Give an outlook on the usage of Context Information for sorting
 */
