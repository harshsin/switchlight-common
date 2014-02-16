#!/usr/bin/python
############################################################
#
# SwitchLight Rest Server
#
############################################################
import sys
import os

from slrest.base.slapi_object import SLAPIObject
from slrest.base import util
from slrest.base import auth
import slrest.api

import cherrypy
from cherrypy import log as cplog

import logging
import logging.handlers
import argparse
import inspect
import logging
import threading
import signal
from cherrypy.process import plugins, servers


class SwitchLightRestServer(object):

    def __init__(self, logger):
        self.logger = logger

    def init(self, ops):
        """Initialize the server and prepare for execution."""

        # Apply optional cherrypy configuration updates. 
        if ops.config:
            if type(ops.config) is str:
                cherrypy.config.update(ops.config)
            elif type(ops.config) is list:
                for c in ops.config:
                    cherrypy.config.update(c)

        # Immediate updates based supported keys on the ops namespace:
        config = {}
        if ops.host:
            config['server.socket_host'] = ops.host

        if ops.port:
            config['server.socket_port'] = int(ops.port)

        if ops.realm:
            config['tools.auth_basic.realm'] = ops.realm

        # Authentication is on by default
        if ops.no_auth is False:
            config['tools.auth_basic.on'] = True
            config['tools.auth_basic.checkpassword'] = auth.checkpassword

        # Authentication override
        if ops.auth:
            # Set username and password
            (username, password) = ops.auth.split(':')
            auth.clear()
            auth.add(username, password)

        # SSL is the default. 
        if ops.no_ssl is False:
            if ops.private_key:
                if os.path.exists(ops.private_key):
                    config['server.ssl_private_key'] = ops.private_key
                else:
                    self.logger.error("The private key file '%s' does not exist." % ops.private_key)
                    return False

            if ops.cert:
                if os.path.exists(ops.cert):
                    config['server.ssl_certificate'] = ops.cert
                else:
                    self.logger.error("The certification file '%s' does not exist." % ops.cert)
                    return False

        # Update manual config
        cherrypy.config.update(config)

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
        ap.add_argument("--cert", help="SSL Certificate.",
                        default='cert.pem')
        ap.add_argument("--private-key", help="SSL Private Key file.",
                        default='privkey.pem')
        ap.add_argument("--realm", help="Authentication Realm.",
                        default="SwitchLight")
        ap.add_argument("--host", help="host on which to listen.",
                        default='0.0.0.0')
        ap.add_argument("--port", help="Listening port.",
                        default=443)
        ap.add_argument("--config", help="Optional CP configuration file.")
        ap.add_argument("--daemonize", help="Run in daemon mode.", action='store_true')
        ap.add_argument("--restart", help="Run in ghetto restart mode.")
        ap.add_argument("--pidfile", help="Write PID file.")
        ap.add_argument("--logfile", help="Write log to logfile.")
        ap.add_argument("--syslog", help="Send logs to syslog.", action='store_true')

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








