#!/usr/bin/python
############################################################
#
# These are the actual REST API implementations. 
#
############################################################
import cherrypy
import logging

from slrest.base.slapi_object import SLAPIObject
from slrest.base import util

class bash(SLAPIObject):
    """Execute a bash command and return the results."""
    route="/api/bash"
    def POST(self, cmd):
        return util.bash_command(cmd)


class pcli(SLAPIObject):
    """Execute a PCLI command and return the results."""
    route="/api/pcli"
    def POST(self, cmd):
        return util.pcli_command(cmd)


class sl_version(SLAPIObject):
    """Get the current SwitchLight version."""
    route="/api/sl_version"
    def GET(self):
        return open("/etc/sl_version").read()


class exit(SLAPIObject):
    """Tell the server to exit."""
    route="/api/server/exit"
    def POST(self,x):
        cherrypy.engine.exit()


class reboot(SLAPIObject):
    """Reboot the system, with delay."""
    route = "/api/system/reboot"
    def POST(self, delay=3):
        d = int(delay)
        util.reboot(self.logger, d)
        return "Rebooting in %s seconds...\n" % d

