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

// begin example "wns.queuingsystem.mm1step1.hpp.example"
#ifndef WNS_QUEUINGSYSTEM_MM1STEP1_HPP
#define WNS_QUEUINGSYSTEM_MM1STEP1_HPP

#include <WNS/rng/RNGen.hpp>
#include <WNS/simulator/ISimulator.hpp>
#include <WNS/simulator/ISimulationModel.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/IOutputStreamable.hpp>
#include <WNS/pyconfig/View.hpp>

#include <boost/bind.hpp>


namespace wns { namespace queuingsystem {

     // Simulation model for queuing system tutorial (step 1).
     //
     // Here We implement our own simulation model. To allow the openwns
     // application to load our custom simulation model, we must inherit
     // from wns::simulator::ISimulationModel.
     //
    class SimpleMM1Step1 :
        public IOutputStreamable,
        public wns::simulator::ISimulationModel
    {
        // Both inter arrival time and processing duration will be
        // exponentially distributed. This defines the type of the
        // corresponding random number generator.
        typedef wns::rng::VariateGenerator< boost::exponential_distribution<> > Exponential;
    public:
        // All simulation models must be constructible from a
        // wns::pyconfig::View.
        explicit
        SimpleMM1Step1(const wns::pyconfig::View& configuration);

    private:
        // Implementation of the ISimulationModel interface.
        // The openwns application will call this method to indicate
        // that construction of the environment (scheduler, random number
        // generator, etc.) has finished.
        //
        // We will place our own startup code here
        virtual void
        doStartup();

        // Implementation of the ISimulationModel interface.
        // The openwns application will call this method to indicate
        // that the simulation ends and the model should shut down.
        //
        // We will place our own shutdown code here
        virtual void
        doShutdown();

        // Implementation of the IOutputStreamable interface.
        // IOutputStreamable implements the stream output operator for
        // us. It will get the contents to output to the stream by
        // calling this method.
        virtual std::string
        doToString() const;

        // Generates a new job that enters the queuing system
        void
        generateNewJob();

        // Excuted after a job has been processed
        void
        onJobProcessed();

        // Starts processing of the next job if one is available
        void
        processNextJob();

        // A random number generator object that creates exponentially
        // distributed random numbers.
        Exponential jobInterarrivalTime_;

        // A random number generator object that creates exponentially
        // distributed random numbers.
        Exponential jobProcessingTime_;

        // Counter for the number of jobs in our system
        int jobsInSystem_;

        // Used to access the logging subsystem
        wns::logger::Logger logger_;
    };
}
}

#endif // NOT defined WNS_QUEUINGSYSTEM_MM1STEP1_HPP
// end example


/**
 * @page wns.queuingsystem.tutorial openWNS Tutorial
 *
 * @section intro Introduction
 *
 * Welcome to the openWNS tutorial. This tutorial consists of several
 * incremental parts that will guide you through all the major parts of
 * the openWNS simulator. We will start with a very simple implementation
 * of a M/M/1 queuing system and will step by step expand it. The following
 * list shows the contents of this tutorial.
 *
 *   -# \subpage wns.queuingsystem.mm1step1 Simplistic MM1 model that
 *   demonstrates the basics of implementing your own simulation model.
 *   -# \subpage wns.queuingsystem.mm1step2 Augments the simplistic model to
 *   support time measurements.
 *   -# \subpage wns.queuingsystem.mm1step3 Introduces the probing subsystem.
 *   -# Shows how to embed your evaluation within your configuration.
 *   -# \subpage wns.queuingsystem.mm1step5 Introduces the ProbeBusRegistry
 *
 * @section mm1queueingsystem The M/M/1 Queuing System
 * The M/M/1 system consists of a queue with unlimited size, a worker with
 * negative-exponentially distributed job processing time. The jobs arrive
 * at the system with an inter arrival time that is negative-exponentially
 * distributed. This is illustrated below
 * @dot
 * digraph overview {
 * rankdir=LR
 * node [shape=record, fontname=Helvetica, fontsize=10, color=black, height=0.2,width=0.4];
 * edge [style="solid", arrowhead="open", fontsize=10, fontname=Helvetica ]
 * ArrivalProcess [shape=ellipse]
 * Worker [shape=ellipse]
 * ArrivalProcess -> Queue
 * Queue -> Worker
 * Worker -> Exit
 * }
 * @enddot
 *
 * @section build Building and Execution of the Examples
 *   - Show directory layout
 *   - Show execution of individual tutorial steps
 *   - Explain which files are involved to allow users to modify these
 *
 * @page wns.queuingsystem.mm1step1 Queuingsystem Tutorial Step 1
 * @section simulationmodel Implementing a SimulationModel
 *
 *   - Describe the relation of main, application and simulation model
 *   - Describe the interface to implement
 *   - Promised environment when doStartup is called
 *
 * @section simplemm1 A very simple M/M/1 queuing system
 *   - Describe the implementation generate job, etc.
 *   - Logging: MESSAGE_BEGIN, MESSAGE_SINGLE
 *   - RNG: Drawing random numbers
 *   - Scheduling: schedule function calls
 *
 * @section configuration Configuration of your system
 *   - PyConfig
 *   - reading values, walk the tree
 * @section loadingsimulationmodel How to tell openWNS which Simulation Model to load
 *
 *   - Registration with StaticFactory
 *   - PyConfig stuff nameInFactory, WNS.setSimulationModel, etc.
 *   - How do I get my config (delegation through the application)
 *
 * @section staticfactory A look behind the scenes (StaticFactory)
 *   - Some basic explanation of the work behind the scenes
 *   - Connect the registration process with the configuration process
 *
 *
 * @include "wns.queuingsystem.mm1step1.hpp.example"
 * @include "wns.queuingsystem.mm1step1.cpp.example"
 */
