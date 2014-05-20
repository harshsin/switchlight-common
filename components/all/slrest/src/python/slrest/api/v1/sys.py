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

