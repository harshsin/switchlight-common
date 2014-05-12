#!/usr/bin/python
############################################################
#
# These are the actual REST API implementations.
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

class bash(SLAPIObject):
    """Execute a bash command and return the results."""
    route="/api/bash"
    def POST(self, cmd):
        return util.bash_command(cmd)[1]


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
        return util.bash_command(cmd)[2]


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
            fn = match.group(1)
            result = cherrypy.lib.static.serve_file(
                fn, content_type='application/gzip',
                disposition='attachment', name=os.path.basename(fn))
            os.unlink(fn)
            return result
        except:
            return ''


class get_controller(SLAPIObject):
    """Get 'show controller' command output."""
    route = "/api/status/controller"
    def GET(self):
        cherrypy.response.headers['Content-Type']= 'text/plain'
        try:
            out = util.pcli_command('show controller')
        except:
            out = ''
        return out

class get_environment(SLAPIObject):
    """Get 'show environment' command output."""
    route = "/api/status/environment"
    def GET(self):
        cherrypy.response.headers['Content-Type']= 'text/plain'
        try:
            out = util.pcli_command('show environment')
        except:
            out = ''
        return out

class get_inventory(SLAPIObject):
    """Get 'show inventory' command output."""
    route = "/api/status/inventory"
    def GET(self):
        cherrypy.response.headers['Content-Type']= 'text/plain'
        try:
            out = util.pcli_command('show inventory')
        except:
            out = ''
        return out

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

        tm = TransactionManagers.get("ZTN", max_=1)
        (tid, tt) = tm.new_task(Discover, self.route)

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

        tm = TransactionManagers.get("ZTN", max_=1)
        (tid, tt) = tm.new_task(TransactUrl, self.route, url)

        if tid is None:
            # Transaction already in progress
            return SLREST.pending(self.route)

        # If the response should be syncronous then join the task:
        if sync:
            tt.join()

        # Return the response
        return tt.response()


class v1_transaction(SLAPIObject):
    """Get the status of the given transaction."""
    route = "/api/v1/transaction"
    def GET(self, id):
        tt = TransactionManagers.get_task(id)
        if tt:
            return tt.response()
        else:
            return SLREST.response(self.route, SLREST.Status.MISSING)

class v1_transactions_running(SLAPIObject):
    """Show all running transaction ids"""
    route = "/api/v1/transactions/running"
    def GET(self):
        return SLREST.response(self.route, SLREST.Status.OK,
                               data=TransactionManagers.get_tids_running())

class v1_transactions_finished(SLAPIObject):
    """Show all finished transaction ids"""
    route = "/api/v1/transactions/finished"
    def GET(self):
        return SLREST.response(self.route, SLREST.Status.OK,
                               data=TransactionManagers.get_tids_finished())

class v1_transactions_all(SLAPIObject):
    """Show all transactions"""
    route = "/api/v1/transactions/all"
    def GET(self):
        return SLREST.response(self.route, SLREST.Status.OK,
                               data=TransactionManagers.get_tids_all())

class v1_sleep(SLAPIObject):
    """
    Simply sleep for the given number of seconds.

    Used for transaction testing.
    """
    route = "/api/v1/sleep"
    def GET(self, seconds, sync=False):

        #
        # Declare a subclass of TransactionTask
        # to contain your task's functionality
        #
        class Sleep(TransactionTask):
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
        #  Its just easiest to specify it here, where we already know the route.
        # The first argument is optional, but will be passed to the subclass
        # as the .args member
        (tid, tt) = tm.new_task(Sleep, self.route, int(seconds))

        # If the response should be syncronous then join the task:
        if sync:
            tt.join()

        # Return the response
        return tt.response()




