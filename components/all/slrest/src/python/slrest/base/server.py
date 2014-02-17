#!/usr/bin/python
############################################################
#
# SwitchLight Rest Server
#
############################################################

from slrest.base.slapi_object import SLAPIObject
from slrest.base import util
from slrest.base import auth
import slrest.api

import cherrypy
from cherrypy import log as cplog
from cherrypy.process import plugins, servers

import sys
import os
import logging
import logging.handlers
import argparse
import inspect
import logging
import threading
import signal
import json


#
# This is a custom tool (in the cherrypy context) to 
# validate the request or reject it based on the
# requestor's source network. 
#
def remote_address_check():
    remote = cherrypy.request.headers['Remote-Addr']
    network = auth.network_check(remote)
    if network:
        # Allowed
        cplog.access_log.error("Remote address %s is from an authorized network (%s)." % (remote, network))
    else:
        # Denied
        cplog.error_log.error("Remote address %s is not from an authorized network." % (remote))
        raise cherrypy.HTTPError(403, "Forbidden")



class SwitchLightRestServer(object):

    def __init__(self, logger):
        self.logger = logger
        self.config = None

    def init(self, ops):
        """Initialize the server and prepare for execution."""

        # Apply optional cherrypy configuration updates. 
        if ops.cp_config:
            if type(ops.cp_config) is str:
                cherrypy.config.update(ops.cp_config)
            elif type(ops.cp_config) is list:
                for c in ops.cp_config:
                    cherrypy.config.update(c)


        if ops.config:
            # Load JSON configuration file
            try:
                self.config = json.load(file(ops.config))
            except Exception, e:
                self.logger.error("Failed to load server configuration file: %s", e)
                return False


        # Apply server settings 
        if self.config:
            if self.config.get('server_accounts'):
                auth.server_account_update(self.config['server_accounts'])
            if self.config.get('user_accounts'):
                auth.user_account_update(self.config['user_accounts'])
            if self.config.get('networks'):
                auth.network_update(self.config['networks'])

        #
        # Cherrypy configuration updates
        # based on the server config file and ops parameters
        #
        cp_config = {}
        if self.config and self.config['cp']:
            cp_config.update(self.config['cp'])

        if ops.host:
            cp_config['server.socket_host'] = ops.host

        if ops.port:
            cp_config['server.socket_port'] = int(ops.port)

        if ops.realm:
            cp_config['tools.auth_basic.realm'] = ops.realm

        # Authentication is on by default
        if ops.no_auth is False:
            cp_config['tools.auth_basic.on'] = True
            cp_config['tools.auth_basic.checkpassword'] = auth.checkpassword

        # Authentication override
        if ops.auth:
            # Set username and password
            (username, password) = ops.auth.split(':')
            auth.server_auth_clear_all()
            auth.server_auth_add(username, password)

        if ops.network:
            auth.network_clear_all()
            auth.network_add("network", ops.network)

        # SSL is the default. 
        if ops.no_ssl is False:
            if ops.private_key:
                if os.path.exists(ops.private_key):
                    cp_config['server.ssl_private_key'] = ops.private_key
                else:
                    self.logger.error("The private key file '%s' does not exist." % ops.private_key)
                    return False

            if ops.cert:
                if os.path.exists(ops.cert):
                    cp_config['server.ssl_certificate'] = ops.cert
                else:
                    self.logger.error("The certification file '%s' does not exist." % ops.cert)
                    return False
            cp_config['server.ssl_module'] = 'builtin'


        # Remote address verification
        cherrypy.tools.remote_address_check = cherrypy.Tool('on_start_resource', remote_address_check)
        cp_config['tools.remote_address_check.on'] = True

        # Update manual config
        cherrypy.config.update(cp_config)


        engine = cherrypy.engine

        if ops.daemonize or ops.logfile or ops.syslog:
            cherrypy.config.update({'log.screen' : False})

        if ops.daemonize:
            plugins.Daemonizer(engine).subscribe()

        if ops.pidfile:
            if not os.path.exists(os.path.dirname(ops.pidfile)):
                os.makedirs(os.path.dirname(ops.pidfile))
            plugins.PIDFile(engine, ops.pidfile).subscribe()
            
        if hasattr(engine, 'signal_handler'):
            engine.signal_handler.subscribe()
            
        # Mount all API implementations. 
        SLAPIObject.mount_all(self.logger)

        return True


    def run(self):
        cherrypy.engine.start()
        cherrypy.engine.block()
        
    @staticmethod
    def main(name="slrest-server"):
        import argparse

        ap = argparse.ArgumentParser(description="SwitchLight REST Server")
        ap.add_argument("--no-ssl", help="Disable SSL (for testing only).",
                        action='store_true')
        ap.add_argument("--no-auth", help="Disable authentication (for testing only).",
                        action='store_true')
        ap.add_argument("--auth", help="Set authentication username and password.")
        ap.add_argument("--cert", help="SSL Certificate.")
        ap.add_argument("--private-key", help="SSL Private Key file.")
        ap.add_argument("--realm", help="Authentication Realm.",
                        default="SwitchLight")
        ap.add_argument("--host", help="host on which to listen.",
                        default='0.0.0.0')
        ap.add_argument("--port", help="Listening port.",
                        default=443)
        ap.add_argument("--cp-config", help="Optional CherryPy configuration file.")
        ap.add_argument("--config", help="Server Config File.")
        ap.add_argument("--daemonize", help="Run in daemon mode.", action='store_true')
        ap.add_argument("--restart", help="Run in ghetto restart mode.")
        ap.add_argument("--pidfile", help="Write PID file.")
        ap.add_argument("--logfile", help="Write log to logfile.")
        ap.add_argument("--syslog", help="Send logs to syslog.", action='store_true')
        ap.add_argument("--network", help="Allow from network.")

        ops = ap.parse_args()

        if ops.logfile:
            if not os.path.exists(os.path.dirname(ops.logfile)):
                os.makedirs(os.path.dirname(ops.logfile))
            logging.basicConfig(filename=ops.logfile)
        else:
            logging.basicConfig()
            
        logger = logging.getLogger(name)
        logger.setLevel(logging.DEBUG)

        if ops.syslog:
            handler = logging.handlers.SysLogHandler(address = '/dev/log')
            logger.addHandler(handler)
            cplog.access_log.addHandler(handler)
            cplog.error_log.addHandler(handler)

        server = SwitchLightRestServer(logger)
        if server.init(ops):
            server.run()
        else:
            # Error already logged
            sys.exit(1)


if __name__ == "__main__":
    SwitchLightRestServer.main()








