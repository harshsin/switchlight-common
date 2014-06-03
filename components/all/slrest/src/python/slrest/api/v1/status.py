#!/usr/bin/python
############################################################
#
# Switch Status
#
############################################################
import cherrypy
import logging
import json

from slrest.base.slapi_object import SLAPIObject
from slrest.base import util
from slrest.base.transact import *
from slrest.base.response import SLREST
from slrest.base import params

import re
import shutil
import traceback


class v1_status_cpu_load(SLAPIObject):
    """Get switch CPU load."""
    route = "/api/v1/status/cpu-load"
    def GET(self, sync=False):
        try:
            with open('/proc/loadavg', 'r') as data:
                avgs = data.readline().split()[0:3]
                out = { '1-min-average': avgs[0],
                        '5-min-average': avgs[1],
                        '15-min-average': avgs[2] }
        except:
            return SLREST.error(self.route,
                                reason='Unable to get cpu load.\n')
        return SLREST.ok(self.route, reason='Command successful.\n',
                         data=json.dumps(out))

class v1_status_memory(SLAPIObject):
    """Get switch memory usage."""
    route = "/api/v1/status/memory"
    def GET(self, sync=False):
        try:
            with open('/proc/meminfo', 'r') as data:
                memtotal = data.readline().split(':')[1].strip()
                memfree = data.readline().split(':')[1].strip()
                out = { 'total': memtotal, 'free': memfree }
        except:
            return SLREST.error(self.route, 
                                reason='Unable to get memory usage.\n')
        return SLREST.ok(self.route, reason='Command successful.\n',
                         data=json.dumps(out))


class v1_status_tech_support(SLAPIObject):
    """Get switch tech support info."""
    route = "/api/v1/status/tech-support"
    def GET(self, sync=False):

        class gen_tech_support_file(TransactionTask):
            def handler(self):
                self.files = {}
                try:
                    out = util.pcli_command('copy tech-support flash2')
                    match = re.search(r'Writing (.*?)\.\.\.', out)
                    src = match.group(1)
                    if os.path.exists(src):
                        dst = os.path.join(self.workdir, os.path.basename(src))
                        # use copyfile/unlink to move file across filesystems
                        shutil.copyfile(src, dst)
                        os.unlink(src)
                        self.status = SLREST.Status.OK
                        self.reason = "Tech support file generated."
                        self.data = self.fidurl('tech-support')
                        self.files['tech-support'] = (os.path.abspath(dst),
                                                      'application/gzip')
                    else:
                        self.status = SLREST.Status.ERROR
                        self.reason = "Tech support file does not exist."
                except:
                    self.status = SLREST.Status.ERROR
                    self.reason = ("Exception generating tech support file:\n"
                                   "%s\n") % (traceback.format_exc())
                self.finish()

        tm = TransactionManagers.get(self.route, 1)

        (tid, tt) = tm.new_task(gen_tech_support_file, self.route)
        if tid is None:
            return SLREST.pending(self.route, 
                                  reason='Another request is pending.\n')

        if sync:
            tt.join()

        return tt.response()


class v1_status_controller(SLAPIObject):
    """Get 'show controller' command output."""
    route = "/api/v1/status/controller"
    def GET(self, sync=False):
        try:
            out = util.pcli_command('show controller')
        except:
            return SLREST.error(self.route, 
                                reason='Unable to get controllers.\n')
        return SLREST.ok(self.route, reason='Command successful.\n',
                         data=out)

class v1_status_environment(SLAPIObject):
    """Get 'show environment' command output."""
    route = "/api/v1/status/environment"
    def GET(self, sync=False):
        try:
            out = util.pcli_command('show environment')
        except:
            return SLREST.error(self.route, 
                                reason='Unable to get environment.\n')
        return SLREST.ok(self.route, reason='Command successful.\n',
                         data=out)

class v1_status_inventory(SLAPIObject):
    """Get 'show inventory' command output."""
    route = "/api/v1/status/inventory"
    def GET(self, sync=False):
        try:
            out = util.pcli_command('show inventory')
        except:
            return SLREST.error(self.route, 
                                reason='Unable to get inventory.\n')
        return SLREST.ok(self.route, reason='Command successful.\n',
                         data=out)

class v1_status_version(SLAPIObject):
    """Get 'show version' command output."""
    route = "/api/v1/status/version"
    def GET(self, sync=False):
        try:
            out = util.pcli_command('show version')
        except:
            return SLREST.error(self.route, 
                                reason='Unable to get version.\n')
        return SLREST.ok(self.route, reason='Command successful.\n',
                         data=out)

