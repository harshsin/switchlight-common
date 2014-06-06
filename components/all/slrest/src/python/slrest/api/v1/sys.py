#!/usr/bin/python
############################################################
#
# System Management
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

class v1_sys_uninstall(SLAPIObject):
    """Uninstall SwitchLight."""
    route = "/api/v1/sys/uninstall"
    def POST(self, factory, reboot):

        (factory, error) = params.boolean('factory', factory)
        if error:
            return SLREST.error(self.route, error)

        (reboot, error) = params.boolean('reboot', reboot)
        if error:
            return SLREST.error(self.route, error)

        if factory:
            # Perform factory reset via ONIE.
            (rc,out) = util.bash_command("fw_setenv onie_boot_reason uninstall")
            if rc:
                return SLREST.error(self.route, reason=out)
        else:
            # Just clean nos_bootcmd. Much faster.
            (rc,out) = util.bash_command("fw_setenv nos_bootcmd echo")
            if rc:
                return SLREST.error(self.route, reason=out)

        if reboot:
            delay=5
            util.reboot(self.logger, delay)
            return SLREST.ok(self.route,
                             reason="Rebooting and uninstalling in %s seconds" % delay)
        else:
            return SLREST.ok(self.route,
                             reason="The system will be uninstalled after the next reboot.")

    @staticmethod
    def cliUninstall(hostname, port, factory, reboot):
        try:
            response = SLAPIObject.post(hostname, port, v1_sys_uninstall.route,
                                        {"factory": factory, "reboot":reboot})
            SLAPIObject.dataResult(response.read())
        except:
             pass

    @staticmethod
    def cmdUninstall(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("uninstall")
            p.add_argument("-factory", action='store_true')
            p.add_argument("-reboot", action='store_true')
            p.set_defaults(func=v1_sys_uninstall.cmdUninstall)
        else:
            v1_sys_uninstall.cliUninstall(sub_parser.hostname, sub_parser.port, sub_parser.factory, sub_parser.reboot)

class v1_sys_reboot(SLAPIObject):
    """Reboot the system, with delay."""
    route = "/api/v1/sys/reboot"
    def POST(self, delay='3'):
        (delay, error) = params.uinteger('delay', delay)
        if error:
            return SLREST.error(self.route, reason=error)

        util.reboot(self.logger, delay)
        return SLREST.ok(self.route,
                         reason="Rebooting in %s seconds...\n" % delay)

    @staticmethod
    def cliReboot(hostname, port, delay):
        try:
            response = SLAPIObject.post(hostname, port, v1_sys_reboot.route, {"delay": delay})
            SLAPIObject.dataResult(response.read())
        except:
            pass

    @staticmethod
    def cmdReboot(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("reboot")
            p.add_argument("delay")
            p.set_defaults(func=v1_sys_reboot.cmdReboot)
        else:
            v1_sys_reboot.cliReboot(sub_parser.hostname, sub_parser.port, sub_parser.delay)

class v1_sys_file_syslog(SLAPIObject):
    """Get the current syslog."""
    route = "/api/v1/sys/file/syslog"
    def GET(self, gzip, sync=False):

        class GetSysLog(TransactionTask):
            def handler(self):
                self.files = {}
                fid='syslog'
                src='/var/log/syslog'
                if self.args:
                    # GZip the file first
                    dst="%s/result.gz" % self.workdir
                    (rc, out) = util.bash_command("gzip -c %s > %s" % (src,dst))
                    if rc:
                        self.status = SLREST.Status.ERROR
                        self.reason = out
                    else:
                        self.files['syslog'] = (os.path.abspath(dst),
                                                'application/gzip')
                        self.status = SLREST.Status.OK
                        self.data=self.fidurl(fid);
                else:
                    self.files['syslog'] = (src,
                                            'application/text')
                    self.status = SLREST.Status.OK
                    self.data=self.fidurl(fid);

                self.finish()


        (gzip, error) = params.boolean('gzip', gzip)
        if error:
            return SLREST.error(self.route, reason=error)

        tm = TransactionManagers.get("SYS")
        (tid, tt) = tm.new_task(GetSysLog, self.route, gzip)
        if sync:
            tt.join()
        return tt.response()

    @staticmethod
    def cliFileSyslog(hostname, port, location, gzip):
        try:
            path = "%s?gzip=%s&sync=True" % (v1_sys_file_syslog.route, gzip)
            response = SLAPIObject.get(hostname, port, path)
            rv = json.loads(response.read())
            if rv['status'] == 'OK':
                result = SLAPIObject.get(hostname, port, rv['data'])
                dst = "%s/syslog.%s" % (location, "gz" if gzip else "txt")
                with open(dst, "w") as f:
                    f.write(result.read())
            else:
                print rv['reason']
        except:
            pass

    @staticmethod
    def cmdFileSyslog(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("syslog")
            p.add_argument("location", help='file storage location')
            p.add_argument("-gzip", action='store_true')
            p.set_defaults(func=v1_sys_file_syslog.cmdFileSyslog)
        else:
            v1_sys_file_syslog.cliFileSyslog(sub_parser.hostname, sub_parser.port, sub_parser.location, sub_parser.gzip)

class v1_sys_beacon(SLAPIObject):
    """Trigger LED beaconing."""
    route = "/api/v1/sys/beacon"
    def POST(self, sync=False):
        # FIXME use platform independent version
        cmd = "ofad-ctl modules brcm led-flash 3 100 100 10"
        (rc, out) = util.bash_command(cmd)
        if rc:
            return SLREST.error(self.route, reason=out)
        return SLREST.ok(self.route,
                         reason='Command successful.\n')

    @staticmethod
    def cliBeacon(hostname, port):
        try:
            response = SLAPIObject.post(hostname, port, v1_sys_beacon.route, {"sync": True})
            SLAPIObject.dataResult(response.read())
        except:
            pass

    @staticmethod
    def cmdBeacon(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("beacon")
            p.set_defaults(func=v1_sys_beacon.cmdBeacon)
        else:
            v1_sys_beacon.cliBeacon(sub_parser.hostname, sub_parser.port)
