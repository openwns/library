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
    'src/Object.cpp',

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
    'src/events/CanTimeout.cpp',
]

srcFiles['EVALUATION'] = [
    'src/evaluation/statistics/stateval.cpp',
    'src/evaluation/statistics/moments.cpp',
    'src/evaluation/statistics/pdf.cpp',
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
    'src/probe/bus/ContextFilterProbeBus.cpp',
    'src/probe/bus/ContextProvider.cpp',
    'src/probe/bus/ContextCollector.cpp',
    'src/probe/bus/LoggingProbeBus.cpp',
    'src/probe/bus/TimeSeriesProbeBus.cpp',
    'src/probe/bus/PassThroughProbeBus.cpp',
    'src/probe/bus/ProbeBus.cpp',
    'src/probe/bus/ProbeBusRegistry.cpp',
    'src/probe/bus/PythonProbeBus.cpp',
    'src/probe/bus/SettlingTimeGuardProbeBus.cpp',
    'src/probe/bus/StatEvalProbeBus.cpp',
    'src/probe/bus/TableProbeBus.cpp',
    'src/probe/bus/TextProbeBus.cpp',
    'src/probe/bus/TimeWindowProbeBus.cpp',
    'src/probe/bus/detail/ObserverPimpl.cpp',
    'src/probe/bus/detail/OutputFormatter.cpp',
    'src/probe/bus/detail/SubjectPimpl.cpp',
    'src/probe/bus/detail/Sorter.cpp',
    'src/probe/bus/detail/StatEvalTable.cpp',
]

srcFiles['TESTING'] = [
    'src/testing/DetailedListener.cpp',
    'src/testing/TestTool.cpp',
    ]
    
srcFiles['OSI'] = [
    'src/osi/PDU.cpp',
    'src/osi/PCI.cpp',
]

srcFiles['NODE'] = [
    'src/node/Node.cpp',
    'src/node/NodeSimulationModel.cpp',
    'src/node/component/Component.cpp',
    'src/node/component/FQSN.cpp',
]   

srcFiles['SERVICE'] = [
    'src/service/tl/PortPool.cpp',
]

srcFiles['DISTRIBUTION'] = [
    'src/distribution/Fixed.cpp',
    'src/distribution/NegExp.cpp',
    'src/distribution/Norm.cpp',
    'src/distribution/Uniform.cpp',
    'src/distribution/DiscreteUniform.cpp',
    'src/distribution/Pareto.cpp',
    'src/distribution/Binomial.cpp',
    'src/distribution/Geometric.cpp',
    'src/distribution/Erlang.cpp',
    'src/distribution/Poisson.cpp',
    'src/distribution/CDFTable.cpp',
    'src/distribution/TimeDependent.cpp',
    'src/distribution/Operation.cpp',
    'src/distribution/Rice.cpp',
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
    'src/tests/AverageTest.cpp',
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
    'src/tests/ObjectTest.cpp',
    'src/tests/IntervalTest.cpp',
    'src/tests/EnumeratorTest.cpp',

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
    'src/container/tests/DynamicMatrixTest.cpp',
    'src/container/tests/PoolTest.cpp',
    'src/container/tests/RangeMapTest.cpp',
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
    'src/probe/bus/tests/ContextFilterProbeBusTest.cpp',
    'src/probe/bus/tests/ContextProviderTest.cpp',
    'src/probe/bus/tests/ContextProviderCollectionTest.cpp',
    'src/probe/bus/tests/PassThroughProbeBusTest.cpp',
    'src/probe/bus/tests/ProbeBusRegistryTest.cpp',
    'src/probe/bus/tests/ProbeBusStub.cpp',
    'src/probe/bus/tests/PythonProbeBusTest.cpp',
    'src/probe/bus/tests/TimeWindowProbeBusTest.cpp',
    'src/probe/bus/tests/TableProbeBusTest.cpp',
    'src/probe/bus/tests/DevelopersGuideTest.cpp',
    'src/probe/bus/detail/tests/SorterTest.cpp',
]

srcFiles['EVALUATION-TESTS'] = [
    'src/evaluation/statistics/tests/StatEvalTest.cpp'
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
    'src/events/tests/CanTimeoutTest.cpp',
    ]
    
srcFiles['OSI-TESTS'] = [
    'src/osi/tests/PCITest.cpp',
    'src/osi/tests/PDUTest.cpp',
]

srcFiles['SERVICE-TESTS'] = [
    'src/service/nl/tests/Address.cpp',
]

srcFiles['NODE-TESTS'] = [
    'src/node/tests/NodeTest.cpp',
    'src/node/tests/NodeHeaderReaderTest.cpp',
    'src/node/tests/Stub.cpp',   
    'src/node/component/tests/ComponentStub.cpp',
    'src/node/component/tests/ComponentTest.cpp',
    'src/node/component/tests/IP.cpp',
    'src/node/component/tests/TCP.cpp',
    'src/node/component/tests/FQSNTest.cpp',
]

srcFiles['SERVICE-TESTS'] = [
    'src/service/tl/tests/PortPoolTest.cpp',
]

srcFiles['DISTRIBUTION-TESTS'] = [
    'src/distribution/tests/FixedTest.cpp',
    'src/distribution/tests/VarEstimator.cpp',
    'src/distribution/tests/NegExpTest.cpp',
    'src/distribution/tests/ErlangTest.cpp',
    'src/distribution/tests/NormTest.cpp',
    'src/distribution/tests/UniformTest.cpp',
    'src/distribution/tests/DiscreteUniformTest.cpp',
    'src/distribution/tests/PoissonTest.cpp',
    'src/distribution/tests/GeometricTest.cpp',
    'src/distribution/tests/ParetoTest.cpp',
    'src/distribution/tests/BinomialTest.cpp',
    'src/distribution/tests/CDFTableTest.cpp',
    'src/distribution/tests/RiceTest.cpp',
    'src/distribution/tests/TimeDependentTest.cpp',
    'src/distribution/tests/OperationTest.cpp',
]

Return('srcFiles')
