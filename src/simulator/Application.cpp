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

#include <WNS/simulator/Application.hpp>
#include <WNS/simulator/SignalHandler.hpp>

#include <WNS/testing/DetailedListener.hpp>
#include <WNS/logger/Master.hpp>
#include <WNS/Assure.hpp>
#include <WNS/TypeInfo.hpp>
#include <WNS/TestFixture.hpp>
#include <WNS/events/scheduler/Interface.hpp>
#include <WNS/simulator/Simulator.hpp>
#include <WNS/simulator/UnitTests.hpp>
#include <WNS/events/MemberFunction.hpp>

#include <WNS/simulator/AbortHandler.hpp>
#include <WNS/simulator/InterruptHandler.hpp>
#include <WNS/simulator/SegmentationViolationHandler.hpp>
#include <WNS/simulator/SignalHandler.hpp>
#include <WNS/simulator/CPUTimeExhaustedHandler.hpp>

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestResult.h>

#include <boost/program_options/value_semantic.hpp>

#include <csignal>
#include <dlfcn.h>
#include <iomanip>
#include <memory>
#include <fstream>
#include <climits>

using namespace wns::simulator;

Application::Application() :
    status_(0),
    configFile_("config.py"),
    configuration_(),
    verbose_(false),
    testing_(false),
    testNames_(),
    pyConfigPatches_(),
    options_(),
    arguments_(),
    programName_("openwns"),
    debuggerName_("gdb"),
    attachDebugger_(false),
    interactiveConfig_(false),
	logger_("WNS", "Application", NULL),
    extendedPrecision_(false)
{
    options_.add_options()

        ("help,?",
         "display this help and exit")

        ("config-file,f",
         boost::program_options::value<std::string>(&configFile_)->default_value("config.py"),
         "load config from configuration file")

        ("attach-debugger-on-segfault,s",
         boost::program_options::value<std::string>(&debuggerName_),
         "fire up gdb on segfault, arg = command for debugger")

        ("stop-in-debugger-on-assure,d",
         boost::program_options::bool_switch(&wns::Assure::useSIGTRAP),
         "stop in debugger if an 'assure' fired (no exception will be thrown)")

        ("interactive-configuration,i",
         boost::program_options::bool_switch(&interactiveConfig_),
         "after reading config start an interactive shell which allows modification of the configuration, use 'continue' to exit shell and run openWNS")

        ("unit-tests,t",
         boost::program_options::bool_switch(&testing_),
         "test mode: run unit tests specified with -T or default suite if no tests with -T given")

        ("named-unit-tests,T",
         boost::program_options::value<TestNameContainer>(&testNames_),
         "run named unit test (defined multipple time for multiple tests), e.g. wns::pyconfig::tests::ParserTest, use with -t")

        ("python-path,P",
         "print Python path and exit")

        ("verbose,v",
         boost::program_options::bool_switch(&verbose_),
         "verbose mode (version information and verbose tests)")

        ("patch-config,y",
         boost::program_options::value<PyConfigPatchContainer>(&pyConfigPatches_),
         "patch the configuration with the given Python expression")

        ("extended-precision",
         boost::program_options::bool_switch(&extendedPrecision_),
         "enabled arithmetic operations with extended precision (80 bit) in x87 (disables strict IEEE754 compatibility)")
        ;
}

Application::~Application()
{
    std::cout << "\nwns::simulator::Application: shutdown complete\n";
}

void
Application::doReadCommandLine(int argc, char* argv[])
{
    programName_ = std::string(argv[0]);

    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, options_),
        arguments_);
    boost::program_options::notify(arguments_);
}

void
Application::doInit()
{
    if (!extendedPrecision_)
    {
        Application::disableX87ExtendedFloatingPointPrecision();
    }

    if (arguments_.count("help") > 0)
    {
        std::cout << options_ << "\n";
        status_ = 0;
        exit(0);
    }

    if (arguments_.count("python-path") > 0)
    {
        std::cout << getPathToPyConfig() << std::endl;
        status_ = 0;
        exit(0);
    }

    // if -s is specified we need to attach the debugger
    attachDebugger_ = arguments_.count("attach-debugger-on-segfault") > 0;

    // patch pyconfig (sys.path, command line patches ...)
    configuration_.appendPath(getPathToPyConfig());
    configuration_.appendPath(".");
    configuration_.load(configFile_);
    for(PyConfigPatchContainer::const_iterator it = pyConfigPatches_.begin();
        it != pyConfigPatches_.end();
        ++it)
    {
        configuration_.patch(*it);
    }

    if(interactiveConfig_)
    {
        configuration_.patch("import pdb\n"
                             "import rlcompleter\n"
                             "pdb.Pdb.complete = rlcompleter.Completer(locals()).complete\n"
                             "pdb.set_trace()\n");
    }

    // after pyconfig is patched, bring up Simulator singelton
    if (testing_)
    {
        wns::simulator::getSingleton().setInstance(new wns::simulator::UnitTests(configuration_));
    }
    else
    {
        wns::simulator::getSingleton().setInstance(new wns::simulator::Simulator(configuration_));
    }

    // reset logger, now set with correct MasterLogger
	pyconfig::View wnsView = configuration_.getView("WNS");
	logger_ = logger::Logger(wnsView.get<wns::pyconfig::View>("logger"));

    // after this we can install the signal handlers as well as the
    // unexpected handlers, because both need the SimulatorSingleton

    // for the backtrace to work we need to set out own unexpected handler
    // for exceptions
    std::set_unexpected(wns::simulator::Application::unexpectedHandler);

    // register signal handlers
    wns::simulator::SignalHandler& handler = wns::simulator::GlobalSignalHandler::Instance();

    // Abort
    handler.addSignalHandler(
        SIGABRT,
        wns::simulator::AbortHandler());

    // Segfault
    handler.addSignalHandler(
        SIGSEGV,
        wns::simulator::SegmentationViolationHandler(
            attachDebugger_,
            debuggerName_,
            programName_));

    // Interrupt (Ctrl+c)
    handler.addSignalHandler(
        SIGINT,
        wns::simulator::InterruptHandler());

    // bring up the Monitor for the event scheduler, may only exists until the
    // master logger is shut down, since it has a logger
    if (!wnsView.isNone("eventSchedulerMonitor"))
    {
        wns::pyconfig::View monitorConfig = wnsView.getView("eventSchedulerMonitor");
        eventSchedulerMonitor_.reset(new wns::events::scheduler::Monitor(monitorConfig));
        eventSchedulerMonitor_->startObserving(wns::simulator::getEventScheduler());
    }
}

void
Application::doRun()
{
    // Unit tests are processed here
    if(testing_)
    {
        CppUnit::TestFactoryRegistry& defaultRegistry = CppUnit::TestFactoryRegistry::getRegistry(wns::testsuite::Default());
        CppUnit::TestSuite* masterSuite = new CppUnit::TestSuite("AllTests");
        CppUnit::TestSuite* defaultSuite = new CppUnit::TestSuite("DefaultTests");
        defaultSuite->addTest(defaultRegistry.makeTest());
        defaultSuite->addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
        masterSuite->addTest(defaultSuite);

        // register disabled tests
        CppUnit::TestFactoryRegistry& disabledRegistry =
            CppUnit::TestFactoryRegistry::getRegistry(wns::testsuite::Disabled());
        masterSuite->addTest(disabledRegistry.makeTest());

        // register performance tests
        CppUnit::TestFactoryRegistry& performanceRegistry =
            CppUnit::TestFactoryRegistry::getRegistry(wns::testsuite::Performance());
        masterSuite->addTest(performanceRegistry.makeTest());

        // register spikes
        CppUnit::TestFactoryRegistry& spikeRegistry =
            CppUnit::TestFactoryRegistry::getRegistry(wns::testsuite::Spike());
        masterSuite->addTest(spikeRegistry.makeTest());

        // setup testrunner
        CppUnit::TextTestRunner runner;
        std::auto_ptr<CppUnit::TestListener> listener;
        if(verbose_)
        {
            listener.reset(new wns::testing::DetailedListener());
        }
        else
        {
            listener.reset(new CppUnit::TextTestProgressListener());
        }
        runner.eventManager().addListener(listener.get());
        runner.addTest(masterSuite);

        // Built tests (either all, or only specific ones given on the
        // command line)
        if(!testNames_.empty())
        {

            for(TestNameContainer::const_iterator ii = testNames_.begin();
                ii != testNames_.end();
                ++ii)
            {
                status_ = runner.run(*ii, false, true, false) ? status_ : 1;
            }
        }
        else
        {
            status_ = runner.run("DefaultTests", false, true, false) ? 0 : 1;
        }

    }
    // normal simulation
    else
    {
		MESSAGE_SINGLE(NORMAL, logger_, "Start Scheduler");

		// install signal handler for graceful shutdown
		wns::simulator::SignalHandler& handler = wns::simulator::GlobalSignalHandler::Instance();
		handler.addSignalHandler(
			SIGXCPU,
			wns::simulator::CPUTimeExhaustedHandler(wns::simulator::getEventScheduler(), SIGXCPU));

        // queue event for end of simulation
        Time maxSimTime = configuration_.get<wns::simulator::Time>("WNS.maxSimTime");
        if (maxSimTime > 0.0)
        {
            wns::events::scheduler::Interface* scheduler = wns::simulator::getEventScheduler();

            typedef wns::events::MemberFunction<wns::events::scheduler::Interface>
                EventSchedulerFunction;

            scheduler->schedule(
                EventSchedulerFunction(
                    scheduler,
                    &wns::events::scheduler::Interface::stop),
                maxSimTime);
        }

		wns::simulator::getEventScheduler()->start();

		// uninstall signal handler for graceful shutdown, can only be
		// used in event loop
		handler.removeSignalHandler(SIGXCPU);
		MESSAGE_SINGLE(NORMAL, logger_, "Simulation finished");
    }
}

void
Application::doShutdown()
{
    // print final stats and shut down the event scheduler monitor
    if (eventSchedulerMonitor_.get() != NULL)
    {
        eventSchedulerMonitor_->logStatistics();
        eventSchedulerMonitor_.reset();
    }

    if (!configuration_.get<bool>("WNS.postProcessing()"))
    {
        throw wns::Exception("WNS.postProcessing() failed!");
    }

    // deregister all signal handler before shutting down master logger
    wns::simulator::GlobalSignalHandler::Instance().removeAllSignalHandlers();

    // restore default state
    std::set_unexpected(std::terminate);

    // Reset the logger, no MasterLogger is set, no output will be available
	logger_ = logger::Logger("WNS", "Application", NULL);

    // This is the very last thing to shut down! Keep the MasterLogger up as
    // long as possible
    wns::simulator::getSingleton().shutdownInstance();

    // Run post processing hook in Python
	if (configuration_.get<bool>("WNS.postProcessing()") == false)
	{
		throw wns::Exception("WNS.postProcessing() failed!");
	}

}


std::string
Application::getPathToPyConfig()
{
    // if this thing here fails you the user can set PYCONFIGPATH ...
    char path[PATH_MAX];
    // /proc/self/exe is a link to the executable (openwns)
    ssize_t length = readlink( "/proc/self/exe", path, PATH_MAX );
    if (length == -1)
    {
        std::cerr << "Warning: could not determine path for PyConfig (readlink('/proc/self/exe') failed.";
        return "./PyConfig";
    }

    // find the last of occurence of '/' and replace with '\0' (terminates the
    // string there). this strips the executable name
    *(strrchr( path, '/' )) = '\0';
    // next to bin we can find the lib dir and below that the PyConfigs
    std::stringstream ss;
    ss << path << "/../lib/PyConfig/";
    return ss.str();
}

int
Application::doStatus() const
{
    return status_;
}

void
Application::unexpectedHandler()
{
    std::cerr << "openWNS: caught an unexpected excpetion!\n";
    wns::simulator::getMasterLogger()->outputBacktrace();
    exit(1);
}

void
Application::disableX87ExtendedFloatingPointPrecision()
{
    unsigned int mode = 0x27F;
	asm("fldcw %0" : : "m" (*&mode));
}
