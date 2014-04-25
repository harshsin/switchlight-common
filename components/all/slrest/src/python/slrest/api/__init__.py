#!/usr/bin/python
############################################################
#
# These are the actual REST API implementations. 
#
############################################################
import cherrypy
import logging
import json
import re

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


class trigger_beacon(SLAPIObject):
    """Trigger LED beaconing."""
    route = "/api/system/beacon"
    def POST(self):
        # FIXME use platform independent version
        cmd = "ofad-ctl modules brcm led-flash 3 100 100 10"
        return util.bash_command(cmd)


class get_cpu_load(SLAPIObject):
    """Get switch CPU load."""
    route = "/api/status/cpu-load"
    def GET(self):
        try:
            with open('/proc/loadavg', 'r') as data:
                avgs = data.readline().split()[0:3]
                out = { '1-min-average': avgs[0],
                        '5-min-average': avgs[1],
                        '15-min-average': avgs[2] }
        except:
            out = {}
        return json.dumps(out)

class get_memory(SLAPIObject):
    """Get switch memory usage."""
    route = "/api/status/memory"
    def GET(self):
        try:
            with open('/proc/meminfo', 'r') as data:
                memtotal = data.readline().split(':')[1].strip()
                memfree = data.readline().split(':')[1].strip()
                out = { 'total': memtotal, 'free': memfree }
        except:
            out = {}
        return json.dumps(out)


class get_tech_support(SLAPIObject):
    """Get switch tech support info."""
    route = "/api/status/tech-support"
    def GET(self):
        try:
            out = util.pcli_command('copy tech-support flash2')
            match = re.search(r'Writing (.*?)\.\.\.', out)
            return open(match.group(1)).read()
        except:
            return ''
