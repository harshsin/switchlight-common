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

    @staticmethod
    def cliZtnInventory(hostname, port):
        try:
            response = SLAPIObject.get(hostname, port, v1_ztn_inventory.route)
            rv = json.loads(response.read())
            if rv['status'] == 'OK' and rv['data'] is not None:
                data = SLAPIObject.fix_unicode(rv['data'])
                for key in data:
                    print key
                    for keys in data[key]:
                        print '%s: %s' % (keys, data[key][keys])
            else:
                print rv['reason']
        except:
            pass

    @staticmethod
    def cmdZtnInventory(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("ztn-inventory")
            p.set_defaults(func=v1_ztn_inventory.cmdZtnInventory)
        else:
            v1_ztn_inventory.cliZtnInventory(sub_parser.hostname, sub_parser.port)

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

    @staticmethod
    def cliZtnDiscover(hostname, port):
        try:
            response = SLAPIObject.post(hostname, port, v1_ztn_discover.route, {"sync": True})
            SLAPIObject.dataResult(response.read()) 
        except:
            pass    

    @staticmethod
    def cmdZtnDiscover(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("ztn-discover")
            p.set_defaults(func=v1_ztn_discover.cmdZtnDiscover)
        else:
            v1_ztn_discover.cliZtnDiscover(sub_parser.hostname, sub_parser.port)

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

    @staticmethod
    def cliZtnTransact(hostname, port, server):
        try:
            response = SLAPIObject.post(hostname, port, v1_ztn_transact_server.route,
                                        {"server": server, "sync": True})
            SLAPIObject.dataResult(response.read())
        except:
             pass

    @staticmethod
    def cmdZtnTransact(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("ztn-transact-server")
            p.add_argument("server")
            p.set_defaults(func=v1_ztn_transact_server.cmdZtnTransact)
        else:
            v1_ztn_transact_server.cliZtnTransact(sub_parser.hostname, sub_parser.port, sub_parser.server)

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

    @staticmethod
    def cliZtnTransactUrl(hostname, port, url):
        try:
            response = SLAPIObject.post(hostname, port, v1_ztn_transact_url.route,
                                        {"url": url, "sync": True})
            SLAPIObject.dataResult(response.read())     
        except:
            pass

    @staticmethod
    def cmdZtnTransactUrl(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("ztn-transact-url")
            p.add_argument("url")
            p.set_defaults(func=v1_ztn_transact_url.cmdZtnTransactUrl)
        else:
            v1_ztn_transact_url.cliZtnTransactUrl(sub_parser.hostname, sub_parser.port, sub_parser.url)

class v1_ztn_reload(SLAPIObject):
    """Perform a config reload."""
    route = "/api/v1/ztn/reload"
    def POST(self, server=None, sync=False):

        class Reload(TransactionTask):
            #
            # This hanlder performs the actual work
            #
            def handler(self):
                (rc, error) = config.reload_config(server)
                if rc:
                    self.status = SLREST.Status.ERROR
                    self.reason = error
                else:
                    self.status = SLREST.Status.OK
                self.finish()

        # Use requester IP as server if server is not provided
        if server is None:
            server = cherrypy.request.remote.ip

        tm = ztn_transaction_manager_get()
        (tid, tt) = tm.new_task(Reload, self.route, server)

        if tid is None:
            # Transaction already in progress
            return SLREST.pending(self.route)

        # If the response should be synchronous then join the task:
        if sync:
            tt.join()

        # Return the response
        return tt.response()

    @staticmethod
    def cliZtnReload(hostname, port):
        try:
            response = SLAPIObject.post(hostname, port, v1_ztn_reload.route,
                                        {"sync": True})
            SLAPIObject.dataResult(response.read())
        except:
            pass

    @staticmethod
    def cmdZtnReload(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("ztn-reload")
            p.set_defaults(func=v1_ztn_reload.cmdZtnReload)
        else:
            v1_ztn_reload.cliZtnReload(sub_parser.hostname, sub_parser.port)
