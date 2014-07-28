#! /usr/bin/python
############################################################
#
# Local Configuration Settings
#
############################################################
import os

LOCALDIR = '/mnt/flash/local.d'

if not os.path.exists(LOCALDIR):
    os.makedirs(LOCALDIR)



#
# These options are represented by touchfiles in LOCALDIR
#

NO_AUTO_REBOOT='no-auto-reboot'
NO_AUTO_RELOAD='no-auto-reload'

OPTIONS = {
    NO_AUTO_REBOOT : {
        'desc' : "Disallow automatic controller reboots.",
        },
    NO_AUTO_RELOAD : {
        'desc' : "Disallow automatic controller configuration reloads.",
        },
    }


def get(name):
    """Return the status of a local configuration setting."""
    return os.path.exists("%s/%s" % (LOCALDIR, name))


