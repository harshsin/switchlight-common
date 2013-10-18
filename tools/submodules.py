#!/usr/bin/python
############################################################
# <bsn.cl fy=2013 v=none>
# 
#        Copyright 2013, 2014 BigSwitch Networks, Inc.        
# 
# 
# 
# </bsn.cl>
############################################################
#
# Submodule management.
#
############################################################
import os
import sys
import subprocess
import shutil
import logging

# The first argument is the set of required modules
required_submodules = sys.argv[1].split(':')

# The second argument is the set of local modules
local_submodules = sys.argv[2].split(':')

# The third argument is the switchlight root
switchlight_root = sys.argv[3]

logging.basicConfig()
logger = logging.getLogger("submodules")
try:
    V = int(os.environ.get('V', '0'))
except ValueError:
    V = 0
if V:
    logger.setLevel(logging.DEBUG)
else:
    logger.setLevel(logging.INFO)

def check_call(cmd, *args, **kwargs):
    if type(cmd) == str:
        logger.debug("+ " + cmd)
    else:
        logger.debug("+ " + " ".join(cmd))
    return subprocess.check_call(cmd, *args, **kwargs)

def check_output(cmd, *args, **kwargs):
    if type(cmd) == str:
        logger.debug("+ " + cmd)
    else:
        logger.debug("+ " + " ".join(cmd))
    return subprocess.check_output(cmd, *args, **kwargs)

def submodule_update(module, depth=None):

    if depth and module != 'loader':
        logger.debug("shallow clone depth=%d", int(depth))
        # Shallow clone first
        url = check_output(['git', 'config', '-f', '.gitmodules', '--get',
                            'submodule.submodules/%s.url' % module])
        url = url.rstrip('\n')
        args = [ 'git', 'clone', '--depth', depth, url, 'submodules/%s' % module ]
        try:
            check_call(args)
        except subprocess.CalledProcessError:
            logger.error("git error cloning module '%s'", module)
            sys.exit(1)

    # full or partial update
    args = [ 'git', 'submodule', 'update', '--init' ]
    if module == 'loader':
        args.append("--recursive")
    args.append('submodules/%s' % module)
    try:
        check_call(args)
    except subprocess.CalledProcessError:
        logger.error("git error updating module '%s'. See the log in %s/submodules/%s.update.log",
                     module, switchlight_root, module)
        sys.exit(1)



#
# Get the current submodule status
#
logger.debug("+ cd %s", switchlight_root)
os.chdir(switchlight_root)

#
# We only operate on the required modules that are also
# defined as local. Any other custom module paths
# are just assumed to be up to the user to manage and instantiate.
#
git_submodule_status = {}
try:
    for entry in check_output(['git', 'submodule', 'status']).split("\n"):
        data = entry.split()
        if len(data) >= 2:
            git_submodule_status[data[1].replace("submodules/", "")] = data[0]
except subprocess.CalledProcessError:
    logger.error("git command(s) failed")
    sys.exit(1)


if '__all__' in required_submodules:
    required_submodules = git_submodule_status.keys()
if '__all__' in local_submodules:
    local_submodules = required_submodules

for module in required_submodules:
    if module in local_submodules:
        status = git_submodule_status[module]
        if status[0] == '-':
            # This submodule has not yet been updated
            if os.path.exists("submodules/%s/modules" % module) or os.path.exists("submodules/%s/Modules" % module):
                # Shudder. The makefiles touched the module manifest as a convenience. That change should be temporary, and so should this one:
                logger.debug("+ /bin/rm -r submodules/%s", module)
                shutil.rmtree("submodules/%s" % module)

            submodule_update(module, os.getenv("SUBMODULE_DEPTH"))
