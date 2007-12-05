import os
import CNBuildSupport
from CNBuildSupport import CNBSEnvironment
import wnsbase.RCS as RCS

commonEnv = CNBSEnvironment(PROJNAME       = 'wns',
                            AUTODEPS       = [],
                            EXAMPLESSEARCH = 'src',
                            LIBRARY        = True,
                            SHORTCUTS      = True,
			    DEFAULTVERSION = True,
			    FLATINCLUDES   = False,
                            LIBS           = ['cppunit',
                                              'dl',
                                              'boost_program_options',
                                              'boost_signals'],
			    REVISIONCONTROL = RCS.Bazaar('../', 'libwns', 'main', '1.0')
                            )

commonEnv.Append(LIBS = ['python'+commonEnv['PYTHONVERSION']])

Return('commonEnv')
