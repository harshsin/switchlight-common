# Copyright (c) 2013  BigSwitch Networks

import os

from sl_util import shell, Service

import command
import run_config

class UnknownServerError(Exception):
    def __init__ (self, server):
        self.server = server

class KnownServerError(Exception):
    def __init__ (self, server):
        self.server = server

class NTP(Service):
    SVC_NAME = "ntp"

class _NTPConfig(object):
    PATH = "/etc/ntp.conf"

    def __init__ (self):
        self._server_cache = set()
        self._cache_tinfo = (0,0)

        firstboot = False
        with open(_NTPConfig.PATH, "r") as cfg:
            try:
                line = cfg.readlines()[0]
                if not line.startswith("### Switch Light"):
                    firstboot = True
            except IndexError, e:
                firstboot = True
        
        if firstboot:
            self._firstboot()

        self._rebuild_cache()

    @property
    def servers (self):
        stat = os.stat(_NTPConfig.PATH)
        if ((stat.st_ctime != self._cache_tinfo[0]) or
            (stat.st_mtime != self._cache_tinfo[1])):
            self._rebuild_cache()
        return self._server_cache
        
    def _rebuild_cache (self):
        ### FIXME: Log this
        stat = os.stat(_NTPConfig.PATH)
        self._cache_tinfo = (stat.st_ctime, stat.st_mtime)

        self._server_cache = set()
        with open(_NTPConfig.PATH, "r") as cfg:
            for line in cfg.readlines():
                if line.startswith("server"):
                    d = line.split()
                    if len(d) >= 2:
                        # Servers with weights will have 4 parts, but we don't preserve
                        # this information (nor provide you a way to set it)
                        self._server_cache.add(d[1])
                    else:
                        ### FIXME: Log something
                        continue
                

    def _firstboot (self):
        with open(_NTPConfig.PATH, "r+") as cfg:
            newcfg = ["### Switch Light\n"]
            for line in cfg.readlines():
                if line.startswith("#") or not line.strip():
                    continue
                newcfg.append(line)
            cfg.seek(0)
            cfg.truncate()
            cfg.write("".join(newcfg))

    def _write_cache (self):
        with open(_NTPConfig.PATH, "w+") as cfg:
            cl = ["### Switch Light\n"]
            # MUST GRAB THE INTERNAL CACHE!
            # We've whacked it by this point, so ctime/mtime will have changed
            for server in self._server_cache:
                cl.append("server %s\n" % (server))
            cfg.write("".join(cl))
            
    def _remove_server (self, server, init):
        ### FIXME: Need a lock
        if server not in self.servers:
            raise UnknownServerError(server)
        self.servers.remove(server)
        self._write_cache()
        NTP.restart(deferred=init)

    def _add_server (self, server, init):
        if server in self.servers:
            raise KnownServerError(server)
        self.servers.add(server)
        self._write_cache()
        NTP.restart(deferred=init)
        

    def cli_config (self, no_command, data, init):
        if no_command:
            try:
                self._remove_server(data["server"], init)
            except UnknownServerError, e:
                print "Server '%s' is not configured" % (data["server"])
        else:
            try:
                self._add_server(data["server"], init)
            except KnownServerError, e:
                print "Server '%s' is already configured" % (data["server"])
            

    def cli_enable (self, no_command, data):
        if no_command:
            NTP.disable()
        else:
            NTP.enable()

    def cli_running_config (self, context, runcfg, words):
        slist = ["ntp server %s\n" % (x) for x in self.servers]
        runcfg.append("!\n")
        if NTP.status() == Service.RUNNING:
            runcfg.append("ntp enable\n")
        else:
            runcfg.append("no ntp enable\n")
        runcfg.extend(slist)


NTPConfig = _NTPConfig()


ntpd_running_config_tuple = (
    (
        {
            'optional'   : False,
            'field'      : 'running-config',
            'type'       : 'enum',
            'values'     : 'ntp',
            'short-help' : 'Configuration for NTP',
            'doc'        : 'running-config|show-ntp',
        },
    ),
)

run_config.register_running_config('ntpd', 2000, None,
                                   NTPConfig.cli_running_config,
                                   ntpd_running_config_tuple)


def show_ntp(data):
    if NTP.status() == Service.RUNNING:
        try:
            shell.call('ntpdc -p', show_output=True)
        except:
            pass
    else:
        print "NTP disabled"

command.add_action('implement-show-ntp', show_ntp,
                    {'kwargs': {'data'      : '$data',}})

SHOW_NTP_COMMAND_DESCRIPTION = {
    'name'          : 'show',
    'mode'          : 'login',
    'no-supported'  : False,
    'args'          : (
        {
            'token'         : 'ntp',
            'action'        : 'implement-show-ntp',
            'short-help'    : 'Show current NTP status',
            'doc'           : 'clock|show',
        },
    )
}
    

command.add_action('implement-config-ntp', NTPConfig.cli_config,
                    {'kwargs' : {
                        'no_command' : '$is-no-command',
                        'data'       : '$data',
                        'init'       : '$is-init',
                    } } )

command.add_action('implement-enable-ntp', NTPConfig.cli_enable,
                    {'kwargs' : {
                        'no_command' : '$is-no-command',
                        'data'       : '$data',
                    } } )
                    

CONFIG_NTP_COMMAND_DESCRIPTION = {
    'name'       : 'ntp',
    'mode'       : 'config',
    'short-help' : 'Configure NTP Settings',
    'action'     : 'implement-config-ntp',
    'no-action'  : 'implement-config-ntp',
    'doc'        : 'clock|ntp',
    'doc-example' : 'clock|ntp-example',
    'args'       : (
        {
            'choices' : (
                (
                    {
                        'token'      : 'enable',
                        'short-help' : 'Set clock via NTP',
                        'action'     : 'implement-enable-ntp',
                        'no-action'  : 'implement-enable-ntp',
                        'doc'        : 'clock|ntp-enable',
                    },
                ),
                (
                    {
                        'token'      : 'server',
                        'short-help' : 'Set NTP Server Address',
                        'doc'        : 'clock|ntp-server',
                    },
                    {
                        'field'           : 'server',
                        'type'            : 'string',
                        'optional-for-no' : False,
                        'syntax-help'     : 'NTP Server Address',
                    },
                ),
            ),
        },
    ),
}
