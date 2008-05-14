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
    'src/module/Version.cpp',
    'src/module/CurrentVersion.cpp',
    'src/module/VersionInformation.cpp',
    'src/module/DependencyList.cpp',
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
    'src/probe/bus/LogEvalProbeBus.cpp',
    'src/probe/bus/PassThroughProbeBus.cpp',
    'src/probe/bus/ProbeBus.cpp',
    'src/probe/bus/ProbeBusRegistry.cpp',
    'src/probe/bus/PythonProbeBus.cpp',
    'src/probe/bus/SettlingTimeGuardProbeBus.cpp',
    'src/probe/bus/TimeWindowProbeBus.cpp',
    'src/probe/bus/detail/ObserverPimpl.cpp',
    'src/probe/bus/detail/SubjectPimpl.cpp',
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
    'src/probe/bus/tests/PassThroughProbeBusTest.cpp',
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

Return('srcFiles')
