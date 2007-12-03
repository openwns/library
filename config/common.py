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
                            LIBS           = ['python2.4', 'sqlite'],
			    REVISIONCONTROL = RCS.GNUArch('../', 'libwns', 'main', '3.0'), 
                            )

Return('commonEnv')
