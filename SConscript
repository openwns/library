import os
import fnmatch
import glob
Import('env installDir includeDir')
libs,headers,pyconfigs = SConscript('config/libfiles.py')

for lib,files in libs.items():
	lib = env.SharedLibrary('wns-' + lib.lower(), files)
	env.Install(installDir, lib )

for config in pyconfigs:
	env.InstallAs(os.path.join(installDir, 'PyConfig', config),
		      os.path.join('PyConfig',config))

