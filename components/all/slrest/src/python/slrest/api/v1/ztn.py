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
import zipfile

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
    def GET(self, sync=True):
        if not sync:
            return SLREST.response(path=self.route,
                                   status=SLREST.Status.ERROR,
                                   reason="async not supported")
        # The ZTN inventory is returned in YAML format.
        (rc, out) = util.bash_command("ztn --inventory")
        if rc:
            return SLREST.response(path=self.route,
                                   status=SLREST.Status.ERROR,
                                   reason=out)

        try:
            d = yaml.load(out)

            # XXX roth -- ha ha, 'date --rfc-3339=seconds' will
            # *almost* represent the date in an RFC3339 format
            # that JodaTime understands
            for n, d2 in d.iteritems():
                for ck, d3 in d2.iteritems():
                    date = d3['date']
                    date = date.replace(' ', 'T')
                    d3['date'] = date

            for ck, d3 in d.get('swi').iteritems():
                path = d3.get('path', None)
                if path is not None:
                    with zipfile.ZipFile(path, "r") as zf:
                        with zf.open("zerotouch.json", "r") as fd:
                            mf = json.loads(fd.read())
                            if 'version' in mf:
                                d3['version'] = mf['version']
                            elif 'release' in mf:
                                d3['version'] = mf['release']
        except Exception, e:
            return SLREST.response(path=self.route,
                                   status=SLREST.Status.ERROR,
                                   reason=str(e))

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
                for typ, typData in data.iteritems():
                    print typ
                    for ck, ckData in typData.iteritems():
                        print "    %s" % ck
                        for key, keyData in ckData.iteritems():
                            print '        %s: %s' % (key, keyData,)
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

class v1_ztn_manifest(SLAPIObject):
    """Get the current ZTN manifest."""
    route = "/api/v1/ztn/manifest"
    def GET(self, sync=True):
        if not sync:
            return SLREST.response(path=self.route,
                                   status=SLREST.Status.ERROR,
                                   reason="async not supported")

        # The ZTN manifest is returned in YAML format.
        (rc, out) = util.bash_command("ztn --manifest")
        if rc:
            return SLREST.response(path=self.route,
                                   status=SLREST.Status.ERROR,
                                   reason=out)

        try:
            d = json.loads(out)
            try:
                d['version'] = open("/etc/sl_version", "r").read().strip()
            except:
                pass
        except Exception, e:
            return SLREST.response(path=self.route,
                                   status=SLREST.Status.ERROR,
                                   reason=str(e))

        return SLREST.response(path=self.route,
                               status=SLREST.Status.OK,
                               data=d)

    @staticmethod
    def cliZtnManifest(hostname, port):
        try:
            response = SLAPIObject.get(hostname, port, v1_ztn_manifest.route)
            rv = json.loads(response.read())
            if rv['status'] == 'OK' and rv['data'] is not None:
                data = SLAPIObject.fix_unicode(rv['data'])
                for key, val in data.iteritems():
                    print '%s: %s' % (key, val,)
            else:
                print rv['reason']
        except:
            pass

    @staticmethod
    def cmdZtnManifest(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("ztn-manifest")
            p.set_defaults(func=v1_ztn_manifest.cmdZtnManifest)
        else:
            v1_ztn_manifest.cliZtnManifest(sub_parser.hostname, sub_parser.port)

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

class v1_ztn_fake_reload(SLAPIObject):
    """Pretend to reload.

    Simply sleep for the given number of seconds.
    """
    route = "/api/v1/ztn/reload"
    def POST(self, sync=False):

        RELOAD_DELAY = 10

        #
        # Declare a subclass of TransactionTask
        # to contain your task's functionality
        #
        class Reload(TransactionTask):
            #
            # This handler performs the actual work
            #
            def handler(self):
                time.sleep(self.args)
                self.status = SLREST.Status.OK
                self.reason = "Sleep %d seconds completed." % self.args
                self.finish()

        # Get a transaction manager. We just instance one per our route
        # Normally this would be per transaction group.
        tm = TransactionManagers.get(self.route)

        # Generate a new task and return the task response.
        # The first argument is the TransactionTask class.
        # The second argument is the path for the response.
        # Its just easiest to specify it here, where we already know the route.
        # The first argument is optional, but will be passed to the subclass
        # as the .args member
        (tid, tt) = tm.new_task(Reload, self.route, RELOAD_DELAY)

        # If the response should be syncronous then join the task:
        if sync:
            tt.join()

        # Return the response
        return tt.response()

    @staticmethod
    def cliReload(hostname, port):
        try:
            path = "%s?sync=True" % (v1_ztn_fake_reload.route)
            response = SLAPIObject.get(hostname, port, path)
            SLAPIObject.dataResult(response.read())
        except:
            pass

    @staticmethod
    def cmdReload(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("reload")
            p.set_defaults(func=v1_ztn_fake_reload.cmdReload)
        else:
            v1_ztn_fake_reload.cliReload(sub_parser.hostname, sub_parser.port)

class v1_ztn_preflight_url(SLAPIObject):
    """Perform the preflight operation.

    Download the selected SWI URL to the ZTN cache.
    """
    route = "/api/v1/ztn/preflight"
    def POST(self, url, sync=False):

        class PreflightUrl(TransactionTask):
            #
            # This handler performs the actual work
            #
            def handler(self):
                (rc, out) = util.bash_command("ztn --add-swi %s" % self.args)
                if rc:
                    self.status = SLREST.Status.ERROR
                else:
                    self.status = SLREST.Status.OK
                self.reason = out
                self.finish()

        tm = ztn_transaction_manager_get()
        (tid, tt) = tm.new_task(PreflightUrl, self.route, url)

        if tid is None:
            # Transaction already in progress
            return SLREST.pending(self.route)

        # If the response should be syncronous then join the task:
        if sync:
            tt.join()

        # Return the response
        return tt.response()

    @staticmethod
    def cliZtnPreflightUrl(hostname, port, url):
        try:
            response = SLAPIObject.post(hostname, port, v1_ztn_preflight_url.route,
                                        {"url": url, "sync": True})
            SLAPIObject.dataResult(response.read())
        except:
            pass

    @staticmethod
    def cmdZtnPreflightUrl(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("ztn-preflight")
            p.add_argument("url")
            p.set_defaults(func=v1_ztn_preflight_url.cmdZtnPreflightUrl)
        else:
            v1_ztn_transact_url.cliZtnPreflightUrl(sub_parser.hostname, sub_parser.port, sub_parser.url)

