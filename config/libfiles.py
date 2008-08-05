import os
srcFiles = dict()

srcFiles['BASE'] = [
    'src/Assure.cpp',
    'src/Exception.cpp',
    'src/TestFixture.cpp',
    'src/StopWatch.cpp',
    'src/Chamaeleon.cpp',
    'src/IOutputStreamable.cpp',
    'src/PythonicOutput.cpp',
    'src/Backtrace.cpp',
    'src/demangle.cpp',

    # module
    'src/module/Base.cpp',
    'src/module/Release.cpp',
    #'src/module/Version.cpp',
    #'src/module/CurrentVersion.cpp',
    #'src/module/VersionInformation.cpp',
    #'src/module/DependencyList.cpp',
    'src/module/DateTime.cpp',
    ]

srcFiles['SIMULATOR'] = [
    'src/simulator/IApplication.cpp',
    'src/simulator/Application.cpp',
    'src/simulator/SignalHandler.cpp',
    'src/simulator/SignalHandlerCallback.cpp',
    'src/simulator/AbortHandler.cpp',
    'src/simulator/SegmentationViolationHandler.cpp',
    'src/simulator/InterruptHandler.cpp',
    'src/simulator/CPUTimeExhaustedHandler.cpp',
    'src/simulator/ISimulator.cpp',
    'src/simulator/Simulator.cpp',
    'src/simulator/UnitTests.cpp',
    'src/simulator/ISimulationModel.cpp',
]

srcFiles['PYCONFIG'] = [
    'src/pyconfig/Object.cpp',
    'src/pyconfig/View.cpp',
    'src/pyconfig/Parser.cpp',
    'src/pyconfig/Sequence.cpp',
    'src/pyconfig/helper/Functions.cpp',
]

srcFiles['EVENTS'] = [
    'src/events/scheduler/IEvent.cpp',
    'src/events/scheduler/Interface.cpp',
    'src/events/scheduler/CommandQueue.cpp',
    'src/events/scheduler/Map.cpp',
    'src/events/scheduler/INotification.cpp',
    'src/events/scheduler/Monitor.cpp',
    'src/events/scheduler/RealTime.cpp',
]

srcFiles['LOGGER'] = [
    ## TODO needs inspection!
    # logger
    'src/logger/Master.cpp',
    'src/logger/Message.cpp',
    'src/logger/Logger.cpp',
    'src/logger/OutputStrategy.cpp',
    'src/logger/CoutOutput.cpp',
    'src/logger/CerrOutput.cpp',
    'src/logger/FileOutput.cpp',
    'src/logger/ConsoleFormat.cpp',
    'src/logger/XMLFormat.cpp',
    'src/logger/DelimiterFormat.cpp',
    'src/logger/SQLiteFormat.cpp',
    'src/logger/FormatStrategy.cpp',

]

srcFiles['PROBEBUS'] = [
    'src/probe/bus/Context.cpp',
    'src/probe/bus/LoggingProbeBus.cpp',
    'src/probe/bus/LogEval.cpp',
    'src/probe/bus/MasterProbeBus.cpp',
    'src/probe/bus/ProbeBus.cpp',
    'src/probe/bus/ProbeBusRegistry.cpp',
    'src/probe/bus/PythonProbeBus.cpp',
    'src/probe/bus/SettlingTimeGuard.cpp',
    'src/probe/bus/TimeWindowProbeBus.cpp',

]

srcFiles['TESTING'] = [
    'src/testing/DetailedListener.cpp',
    ]

srcFiles['QUEUINGSYSTEM'] = [
    'src/queuingsystem/Job.cpp',
    'src/queuingsystem/MM1Step1.cpp',
    'src/queuingsystem/MM1Step2.cpp',
    'src/queuingsystem/MM1Step3.cpp',
    'src/queuingsystem/MM1Step5.cpp',
    'src/queuingsystem/MM1Step6.cpp',
    ]

srcFiles['BASE-TESTS'] = [
    'src/tests/AssureTest.cpp',
    'src/tests/ChamaeleonTest.cpp',
    'src/tests/ExceptionTest.cpp',
    'src/tests/TypeInfoTest.cpp',
    'src/tests/FunctorTest.cpp',
    'src/tests/StaticFactoryTest.cpp',
    'src/tests/SmartPtrTest.cpp',
    'src/tests/SmartPtrWithDebuggingTest.cpp',
    'src/tests/PythonicOutputTest.cpp',
    'src/tests/StopWatchTest.cpp',
    'src/tests/BacktraceTest.cpp',
    'src/tests/ObserverTest.cpp',

    'src/module/tests/ModuleTest.cpp',
    'src/module/tests/MultiTypeFactoryTest.cpp',
]

srcFiles['SIMULATOR-TESTS'] = [
    'src/simulator/tests/MainTest.cpp',
    ]

srcFiles['CONTAINER-TESTS'] = [
    'src/container/tests/FastListTest.cpp',
    'src/container/tests/UntypedRegistryTest.cpp',
    'src/container/tests/RegistryTest.cpp',
]

srcFiles['PYCONFIG-TESTS'] = [
    'src/pyconfig/tests/ParserTest.cpp',
    'src/pyconfig/tests/ViewTest.cpp',
    'src/pyconfig/tests/SequenceTest.cpp',
    'src/pyconfig/helper/tests/FunctionsTest.cpp',
]

srcFiles['LOGGER-TESTS'] = [
    'src/logger/tests/MasterTest.cpp',
    'src/logger/tests/MessageTest.cpp',
    'src/logger/tests/LoggerTest.cpp',
    'src/logger/tests/LoggerTestHelper.cpp',
]
srcFiles['PROBEBUS-TESTS'] = [
    'src/probe/bus/tests/ContextTest.cpp',
    'src/probe/bus/tests/MasterProbeBusTest.cpp',
    'src/probe/bus/tests/ProbeBusStub.cpp',
    'src/probe/bus/tests/PythonProbeBusTest.cpp',
    'src/probe/bus/tests/TimeWindowProbeBusTest.cpp',
]

srcFiles['EVENTS-TESTS'] = [
    'src/events/tests/MemberFunctionTest.cpp',
    'src/events/tests/DelayedMemberFunctionTest.cpp',

    'src/events/scheduler/tests/CallableTest.cpp',
    'src/events/scheduler/tests/InterfaceTest.cpp',
    'src/events/scheduler/tests/MapInterfaceTest.cpp',
    'src/events/scheduler/tests/PerformanceTest.cpp',
    'src/events/scheduler/tests/MapPerformanceTest.cpp',
    'src/events/scheduler/tests/BestPracticesTest.cpp',
    'src/events/scheduler/tests/RealTimeTest.cpp',
    ]

hppFiles = [
'src/queuingsystem/Job.hpp',
'src/queuingsystem/MM1Step1.hpp',
'src/queuingsystem/MM1Step6.hpp',
'src/queuingsystem/MM1Step3.hpp',
'src/queuingsystem/MM1Step5.hpp',
'src/queuingsystem/MM1Step2.hpp',
'src/Assure.hpp',
'src/LongCreator.hpp',
'src/rng/RNGen.hpp',
'src/Singleton.hpp',
'src/RefCountable.hpp',
'src/Chamaeleon.hpp',
'src/logger/CoutOutput.hpp',
'src/logger/FormatStrategy.hpp',
'src/logger/SQLiteFormat.hpp',
'src/logger/OutputStrategy.hpp',
'src/logger/Master.hpp',
'src/logger/tests/LoggerTest.hpp',
'src/logger/tests/MasterTest.hpp',
'src/logger/tests/LoggerTestHelper.hpp',
'src/logger/tests/MessageTest.hpp',
'src/logger/FileOutput.hpp',
'src/logger/ConsoleFormat.hpp',
'src/logger/CerrOutput.hpp',
'src/logger/XMLFormat.hpp',
'src/logger/Message.hpp',
'src/logger/DelimiterFormat.hpp',
'src/logger/Logger.hpp',
'src/NonCopyable.hpp',
'src/SubjectInterface.hpp',
'src/SmartPtr.hpp',
'src/events/NoOp.hpp',
'src/events/MemberFunction.hpp',
'src/events/scheduler/CommandQueue.hpp',
'src/events/scheduler/Map.hpp',
'src/events/scheduler/Callable.hpp',
'src/events/scheduler/ICommand.hpp',
'src/events/scheduler/tests/InterfaceTest.hpp',
'src/events/scheduler/tests/PerformanceTest.hpp',
'src/events/scheduler/RealTime.hpp',
'src/events/scheduler/NullCommand.hpp',
'src/events/scheduler/Monitor.hpp',
'src/events/scheduler/INotification.hpp',
'src/events/scheduler/IEvent.hpp',
'src/events/scheduler/Interface.hpp',
'src/SmartPtrBase.hpp',
'src/Python.hpp',
'src/Subject.hpp',
'src/Functor.hpp',
'src/IOutputStreamable.hpp',
'src/PyConfigViewCreator.hpp',
'src/module/Module.hpp',
'src/module/tests/ModuleTest.hpp',
'src/module/tests/MultiTypeFactoryTest.hpp',
'src/module/CurrentVersion.hpp',
'src/module/DateTime.hpp',
'src/module/DependencyList.hpp',
'src/module/Version.hpp',
'src/module/VersionInformation.hpp',
'src/module/Release.hpp',
'src/module/MultiTypeFactory.hpp',
'src/module/Base.hpp',
'src/Backtrace.hpp',
'src/simulator/Bit.hpp',
'src/simulator/Main.hpp',
'src/simulator/InterruptHandler.hpp',
'src/simulator/CPUTimeExhaustedHandler.hpp',
'src/simulator/ISimulator.hpp',
'src/simulator/IApplication.hpp',
'src/simulator/UnitTests.hpp',
'src/simulator/ISimulationModel.hpp',
'src/simulator/Time.hpp',
'src/simulator/SignalHandlerCallback.hpp',
'src/simulator/AbortHandler.hpp',
'src/simulator/Application.hpp',
'src/simulator/SignalHandler.hpp',
'src/simulator/SegmentationViolationHandler.hpp',
'src/simulator/Simulator.hpp',
'src/Exception.hpp',
'src/probe/bus/PythonProbeBus.hpp',
'src/probe/bus/tests/ProbeBusStub.hpp',
'src/probe/bus/SettlingTimeGuard.hpp',
'src/probe/bus/LoggingProbeBus.hpp',
'src/probe/bus/LogEval.hpp',
'src/probe/bus/ProbeBus.hpp',
'src/probe/bus/ProbeBusRegistry.hpp',
'src/probe/bus/Context.hpp',
'src/probe/bus/MasterProbeBus.hpp',
'src/probe/bus/TimeWindowProbeBus.hpp',
'src/PythonicOutput.hpp',
'src/pyconfig/helper/tests/FunctionsTest.hpp',
'src/pyconfig/helper/Functions.hpp',
'src/pyconfig/Parser.hpp',
'src/pyconfig/Converter.hpp',
'src/pyconfig/tests/SequenceTest.hpp',
'src/pyconfig/tests/ViewTest.hpp',
'src/pyconfig/tests/ParserTest.hpp',
'src/pyconfig/View.hpp',
'src/pyconfig/Sequence.hpp',
'src/pyconfig/Object.hpp',
'src/testing/DetailedListener.hpp',
'src/ObserverInterface.hpp',
'src/TypeInfo.hpp',
'src/StaticFactory.hpp',
'src/container/FastListEnabler.hpp',
'src/container/Registry.hpp',
'src/container/FastListNode.hpp',
'src/container/FastList.hpp',
'src/container/UntypedRegistry.hpp',
'src/Conversion.hpp',
'src/TestFixture.hpp',
'src/demangle.hpp',
'src/StopWatch.hpp',
'src/Observer.hpp',
]

pyconfig = [
'openwns/simulator.py',
'openwns/queuingsystem.py',
'openwns/eventscheduler.py',
'openwns/rng.py',
'openwns/tests/simulatorTest.py',
'openwns/tests/__init__.py',
'openwns/module.py',
'openwns/logger.py',
'openwns/probebus.py',
'openwns/interface.py',
'openwns/pyconfig.py',
'openwns/__init__.py',
'openwns/backend/pyconfig.py',
'openwns/backend/__init__.py',

]

Return('srcFiles hppFiles pyconfig')
