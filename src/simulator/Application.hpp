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

#ifndef WNS_SIMULATOR_APPLICATION_HPP
#define WNS_SIMULATOR_APPLICATION_HPP

#include <WNS/simulator/IApplication.hpp>
#include <WNS/pyconfig/Parser.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/events/scheduler/Monitor.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <vector>
#include <string>
#include <memory>

namespace wns { namespace simulator {

    /**
     * @brief Run-time environment of openWNS
     * @author Marc Schinnenburg <marc@schinnenburg.com>
     *
     * The following mehtods are called in this order:
     *  - Application()
     *  - readCommandLine()
     *  - init()
     *  - run()
     *  - shutdown()
     *  - status()
     *  - ~Application()
     */
    class Application :
        public IApplication
    {
        // needs to be vector to work with boost
        typedef std::vector<std::string> TestNameContainer;
        typedef std::vector<std::string> PyConfigPatchContainer;

    public:
        /**
         * @brief Default Constructor
         */
        Application();

        /**
         * @brief Default Destructor
         */
        virtual
        ~Application();

    protected:
        /**
         * @brief Reads the command line parameters
         */
        virtual void
        doReadCommandLine(int argc, char* argv[]);

        /**
         * @brief Configure stuff that belongs to WNS
         */
        virtual void
        doInit();

        /**
         * @brief Run the simulation
         */
        virtual void
        doRun();

        /**
         * @brief Shutdown everything
         */
        virtual void
        doShutdown();

        /**
         * @brief Return status code
         */
        virtual int
        doStatus() const;

        /**
         * @brief Handle unexpected exceptions
         */
        static void
        unexpectedHandler();

        /**
         * @brief Returns the path to pyconfig
         */
        std::string
        getPythonPath() const;

        /**
         * @brief The status code of openWNS
         *
         * 0 - everything fine
         *
         * 1 - an error occured
         */
        int status_;

        /**
         * @brief Name of the configuration file
         */
        std::string configFile_;

        /**
         * @brief Configuration as parsed from configFile_
         */
        pyconfig::Parser configuration_;

        /**
         * @brief Be chatty
         */
        bool verbose_;

        /**
         * @brief Is wns in testing mode?
         */
        bool testing_;

        /**
         * @brief If special tests have been defined they are stored here
         */
        TestNameContainer testNames_;

        /**
         * @brief Holds all PyConfig patches
         */
        PyConfigPatchContainer pyConfigPatches_;

        /**
         * @brief Defines the valid options for the command line
         */
        boost::program_options::options_description options_;

        /**
         * brief Holds the command line parameters
         */
        boost::program_options::variables_map arguments_;

        /**
         * @brief Our own name (as from argv[0])
         */
        std::string programName_;

        /**
         * @brief In case we should launch a debugger, this is the name of it
         */
        std::string debuggerName_;

        /**
         * @brief Are we expected to launch a debugger on segfault?
         */
        bool attachDebugger_;

        /**
         * @brief Interactive config
         */
        bool interactiveConfig_;

        /**
         * @brief The logger of the Application
         *
         * @note The logger can only be used after init and before shutdown
         */
		wns::logger::Logger logger_;

        /**
         * @brief a monitor for the event scheduler
         */
        std::auto_ptr<wns::events::scheduler::Monitor> eventSchedulerMonitor_;
    };

} // simulator
} // wns

#endif // NOT defined WNS_SIMULATOR_APPLICATION_HPP

