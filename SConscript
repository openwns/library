import os
Import('env')
srcFiles,headers,pyconfigs = SConscript('config/libfiles.py')

if len(srcFiles) != 0:
    lib = env.SharedLibrary('wns', srcFiles)
    env.Install(os.path.join(env.installDir, 'lib'), lib )

for config in pyconfigs:
    env.InstallAs(os.path.join(env.installDir, 'lib', 'PyConfig', config),
                  os.path.join('PyConfig',config))

