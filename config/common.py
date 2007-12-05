import os
import sys
import CNBuildSupport
from CNBuildSupport import CNBSEnvironment
import wnsbase.RCS as RCS

# libwns needs special support to find Python headers and link against
# the correct version of libpython. By default, we assume the version
# to be linked against is the version this script is running
pythonVersion = sys.version[:3]

# in order to be able to modify the path in the private.py (if needed)
# the verision the include path and the python lib name are added to
# the environment as PYTHONVERSION, PYTHONINCLUDEPATH and PYTHONLIB.

# One can say
#    env['LIBS'].pop(env['PYTHONLIB'])
# to remove the version
# specified by this configuration and
#    env.Append(LIBS="python2.5")
# afterwards to specify a different version The same applies to the
# python include path.

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
			    REVISIONCONTROL = RCS.Bazaar('../', 'libwns', 'main', '1.0'),
                            # set the python version
                            PYTHONVERSION = pythonVersion,
                            # then the default path to serach in is
                            PYTHONINCLUDEPATH = os.path.join(os.sep, 'usr', 'include', 'python'+pythonVersion),
                            PYTHONLIB = 'python'+pythonVersion
                            )

commonEnv.Append(CPPPATH = [commonEnv['PYTHONINCLUDEPATH']])
commonEnv.Append(LIBS    = [commonEnv['PYTHONLIB']])

CNBuildSupport.printINF("Lib will link against Python version:  " + pythonVersion)
CNBuildSupport.printINF("Header files for Python will be used from:  " + commonEnv['PYTHONINCLUDEPATH'])
CNBuildSupport.printINF("See config/common.py if you need to change this.")

Return('commonEnv')
