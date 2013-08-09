#!/usr/bin/python
###############################################################################
#
# Submodule management. 
#
###############################################################################
import os
import sys
import subprocess

# The first argument is the set of required modules
required_submodules = sys.argv[1].split(':')

# The second argument is the set of local modules
local_submodules = sys.argv[2].split(':')

# The third argument is the switchlight root
switchlight_root = sys.argv[3]

# 
# Get the current submodule status
#
os.chdir(switchlight_root)

#
# We only operate on the required modules that are also
# defined as local. Any other custom module paths
# are just assumed to be up to the user to manage and instantiate. 
#
git_submodule_status = {}
try:
    for entry in subprocess.check_output(['git', 'submodule', 'status']).split("\n"):
        data = entry.split()
        if len(data) >= 2:
            git_submodule_status[data[1].replace("submodules/", "")] = data[0]
except Exception as e:
    print repr(e)
    raise

for module in required_submodules:
    if module in local_submodules:
        status = git_submodule_status[module]
        if status[0] == '-':
            # This submodule has not yet been updated
            if subprocess.check_call(['git', 'submodule', 'update', '--init', '--recursive', 'submodules/%s' % module]) != 0:
                print "git error updating module '%s'. See the log in %s/submodules/%s.update.log" % (module, switchlight_root, module)
                sys.exit(1)






