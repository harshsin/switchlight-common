#!/usr/bin/python
############################################################
#
# ZTN Support
#
############################################################
import cherrypy
import logging
import json
import yaml
import re
import os

from slrest.base.slapi_object import SLAPIObject
from slrest.base import util
from slrest.base import config
from slrest.base.transact import *
from slrest.base.response import SLREST


#
# Only one ZTN transaction may be outstanding at a time.
#
# These are all submitted under the "ZTN" transaction manager.
#
ZTN_TRANSACTION_MANAGER="ZTN"
ZTN_TRANSACTION_MANAGER_MAX=1

def ztn_transaction_manager_get():
    return TransactionManagers.get(ZTN_TRANSACTION_MANAGER,
                                   ZTN_TRANSACTION_MANAGER_MAX)


class v1_ztn_inventory(SLAPIObject):
    """Get the current ZTN inventory."""
    route = "/api/v1/ztn/inventory"
    def GET(self):
        # The ZTN inventory is returned in YAML format.
        (rc, out) = util.bash_command("ztn --inventory")
        if rc:
            return SLREST.response(path=self.route,
                                   status=SLREST.Status.ERROR,
                                   reason=out)
        else:
            d = yaml.load(out)
            return SLREST.response(path=self.route,
                                   status=SLREST.Status.OK,
                                   data=d)

class v1_ztn_discover(SLAPIObject):
    """Run ZTN discovery."""
    route = "/api/v1/ztn/discover"
    def POST(self, sync=False):
        class Discover(TransactionTask):
            #
            # This handler performs the actual work
            #
            def handler(self):
                (rc, out) = util.bash_command("ztn --discover")
                if rc:
                    self.status = SLREST.Status.ERROR
                else:
                    self.status = SLREST.Status.OK
                self.reason = out
                self.finish()

        tm = ztn_transaction_manager_get()

        (tid, tt) = tm.new_task(Discover, self.route)

        if tid is None:
            # Transaction already in progress
            return SLREST.pending(self.route)

        # If the response should be syncronous then join the task:
        if sync:
            tt.join()

        # Return the response
        return tt.response()


class v1_ztn_transact_server(SLAPIObject):
    """Perform a manifest transaction using the given server."""
    route = "/api/v1/ztn/transact/server"
    def POST(self, server, sync=False):

        class TransactServer(TransactionTask):
            #
            # This handler performs the actual work
            #
            def handler(self):
                (rc, out) = util.bash_command("ztn --transact --server %s" % self.args)
                if rc:
                    self.status = SLREST.Status.ERROR
                else:
                    self.status = SLREST.Status.OK
                self.reason = out
                self.finish()

        tm = ztn_transaction_manager_get()
        (tid, tt) = tm.new_task(TransactServer, self.route, server)

        if tid is None:
            # Transaction already in progress
            return SLREST.pending(self.route)

        # If the response should be syncronous then join the task:
        if sync:
            tt.join()

        # Return the response
        return tt.response()



class v1_ztn_transact_url(SLAPIObject):
    """Perform a manifest transaction using the given URL."""
    route = "/api/v1/ztn/transact/url"
    def POST(self, url, sync=False):

        class TransactUrl(TransactionTask):
            #
            # This handler performs the actual work
            #
            def handler(self):
                (rc, out) = util.bash_command("ztn --transact --url %s" % self.args)
                if rc:
                    self.status = SLREST.Status.ERROR
                else:
                    self.status = SLREST.Status.OK
                self.reason = out
                self.finish()

        tm = ztn_transaction_manager_get()
        (tid, tt) = tm.new_task(TransactUrl, self.route, url)

        if tid is None:
            # Transaction already in progress
            return SLREST.pending(self.route)

        # If the response should be syncronous then join the task:
        if sync:
            tt.join()

        # Return the response
        return tt.response()


