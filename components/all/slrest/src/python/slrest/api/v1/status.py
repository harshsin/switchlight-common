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
                avgs = [float(x) for x in data.readline().split()[0:3]]
                out = { '1-min-average': avgs[0],
                        '5-min-average': avgs[1],
                        '15-min-average': avgs[2] }
        except:
            return SLREST.error(self.route,
                                reason='Unable to get cpu load.\n')
        return SLREST.ok(self.route, reason='Command successful.\n',
                         data=out)

    @staticmethod
    def cliCpuLoad(hostname, port):
        try:
            path = "%s?sync=True" % v1_status_cpu_load.route
            response = SLAPIObject.get(hostname, port, path)
            SLAPIObject.dataResult(response.read())
        except:
            pass

    @staticmethod
    def cmdCpuLoad(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("cpu-load")
            p.set_defaults(func=v1_status_cpu_load.cmdCpuLoad)
        else:
            v1_status_cpu_load.cliCpuLoad(sub_parser.hostname, sub_parser.port)

class v1_status_memory(SLAPIObject):
    """Get switch memory usage."""
    route = "/api/v1/status/memory"
    def GET(self, sync=False):

        def _tup(line):
            key, val = line.split(':')
            val = val.strip()
            if val.endswith(' kB'):
                return (key, 1024*int(val[:-3]),)
            else:
                return (key, val)

        try:
            with open('/proc/meminfo', 'r') as fd:
                meminfo = dict(map(_tup, fd.read().strip().splitlines()))
                out = dict(total=meminfo['MemTotal'],
                           free=meminfo['MemFree'])
        except Exception, e:
            msg = "Unable to get memory usage: %s" % str(e)
            return SLREST.error(self.route, reason=msg)
        return SLREST.ok(self.route,
                         reason='Command successful.\n',
                         data=out)

    @staticmethod
    def cliMemory(hostname, port):
        try:
            path = "%s?sync=True" % v1_status_memory.route
            response = SLAPIObject.get(hostname, port, path)
            SLAPIObject.dataResult(response.read())
        except:
            pass

    @staticmethod
    def cmdMemory(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("memory")
            p.set_defaults(func=v1_status_memory.cmdMemory)
        else:
            v1_status_memory.cliMemory(sub_parser.hostname, sub_parser.port)

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

    @staticmethod
    def cliTechSupport(hostname, port, location):
        try:
            path = "%s?sync=True" % v1_status_tech_support.route 
            response = SLAPIObject.get(hostname, port, path)
            rv = json.loads(response.read())
            if rv['status'] == 'OK':
                result = SLAPIObject.get(hostname, port, rv['data'])
                dst = "%s/tech-support.gz" % location
                with open(dst, "w") as f:
                    f.write(result.read())
            else:
                print rv['reason']
        except:
            pass
    
    @staticmethod
    def cmdTechSupport(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("tech-support")
            p.add_argument("location", help='file storage location')
            p.set_defaults(func=v1_status_tech_support.cmdTechSupport)
        else:
            v1_status_tech_support.cliTechSupport(sub_parser.hostname, sub_parser.port, sub_parser.location)

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

    @staticmethod
    def cliController(hostname, port):
        try:
            path = "%s?sync=True" % v1_status_controller.route
            response = SLAPIObject.get(hostname, port, path)
            SLAPIObject.dataResult(response.read())
        except:
            pass

    @staticmethod
    def cmdController(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("controller")
            p.set_defaults(func=v1_status_controller.cmdController)
        else:
            v1_status_controller.cliController(sub_parser.hostname, sub_parser.port)

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

    @staticmethod
    def cliEnvironment(hostname, port):
        try:
            path = "%s?sync=True" % v1_status_environment.route
            response = SLAPIObject.get(hostname, port, path)
            SLAPIObject.dataResult(response.read())
        except:
            pass

    @staticmethod
    def cmdEnvironment(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("environment")
            p.set_defaults(func=v1_status_environment.cmdEnvironment)
        else:
            v1_status_environment.cliEnvironment(sub_parser.hostname, sub_parser.port)

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

    @staticmethod
    def cliInventory(hostname, port):
        try:
            path = "%s?sync=True" % v1_status_inventory.route
            response = SLAPIObject.get(hostname, port, path)
            SLAPIObject.dataResult(response.read())
        except:
            pass

    @staticmethod
    def cmdInventory(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("inventory")
            p.set_defaults(func=v1_status_inventory.cmdInventory)
        else:
            v1_status_inventory.cliInventory(sub_parser.hostname, sub_parser.port)

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

    @staticmethod
    def cliVersion(hostname, port):
        try:
            path = "%s?sync=True" % v1_status_version.route
            response = SLAPIObject.get(hostname, port, path)
            SLAPIObject.dataResult(response.read())
        except:
            pass

    @staticmethod
    def cmdVersion(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("version")
            p.set_defaults(func=v1_status_version.cmdVersion)
        else:
            v1_status_version.cliVersion(sub_parser.hostname, sub_parser.port)

class v1_status_ping(SLAPIObject):
    """Respond to a simple 'ping'."""
    route = "/api/v1/status/ping"
    def GET(self, sync=True):
        return SLREST.ok(self.route, reason='Command successful.\n')

    @staticmethod
    def cliPing(hostname, port):
        try:
            path = "%s?sync=True" % v1_status_ping.route
            response = SLAPIObject.get(hostname, port, path)
            SLAPIObject.dataResult(response.read())
        except:
            pass

    @staticmethod
    def cmdPing(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("ping")
            p.set_defaults(func=v1_status_ping.cmdPing)
        else:
            v1_status_ping.cliPing(sub_parser.hostname, sub_parser.port)
