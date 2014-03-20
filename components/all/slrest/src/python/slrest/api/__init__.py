#!/usr/bin/python
############################################################
#
# These are the actual REST API implementations. 
#
############################################################
import cherrypy
import logging
import json

from slrest.base.slapi_object import SLAPIObject
from slrest.base import util
from slrest.base import config

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

class update_sl_config(SLAPIObject):
    """Update and reload switch startup and running configs."""
    route = "/api/sl_config/update"
    def POST(self, url, async=False):
        transaction_id = config.start_update_config_task(url)
        if async:
            return json.dumps({"transaction_id": transaction_id})
        else:
            config.wait_for_update_config_task()
            return json.dumps(config.get_update_config_status(transaction_id))

class get_update_sl_config_status(SLAPIObject):
    """Get status on update config tasks."""
    route = "/api/sl_config/update_status"
    def GET(self, transaction_id):
        return json.dumps(config.get_update_config_status(int(transaction_id)))

class get_startup_sl_config(SLAPIObject):
    """Get switch startup config."""
    route = "/api/sl_config/get_startup"
    def GET(self):
        return "\n".join(config.get_startup_config()) + "\n"

class get_running_sl_config(SLAPIObject):
    """Get switch running config."""
    route = "/api/sl_config/get_running"
    def GET(self):
        return "\n".join(config.get_running_config()) + "\n"