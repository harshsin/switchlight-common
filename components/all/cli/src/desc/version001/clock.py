# Copyright (c) 2013-2014  BigSwitch Networks

import os

from sl_util import shell, Service, const, conf_state

import command
import run_config
import signal
import subprocess

class UnknownServerError(Exception):
    def __init__ (self, server):
        self.server = server

class KnownServerError(Exception):
    def __init__ (self, server):
        self.server = server

class NTP(Service):
    SVC_NAME = "ntp"
    CFG_PATH = const.NTP_CFG_PATH

conf_state.register_save("ntp", NTP.save_default_settings)
conf_state.register_revert("ntp", NTP.revert_default_settings)

class _NTPConfig(object):
    def __init__ (self):
        self._server_cache = set()
        self._cache_tinfo = (0,0)

        firstboot = False
        with open(NTP.CFG_PATH, "r") as cfg:
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
        stat = os.stat(NTP.CFG_PATH)
        if ((stat.st_ctime != self._cache_tinfo[0]) or
            (stat.st_mtime != self._cache_tinfo[1])):
            self._rebuild_cache()
        return self._server_cache
        
    def _rebuild_cache (self):
        ### FIXME: Log this
        stat = os.stat(NTP.CFG_PATH)
        self._cache_tinfo = (stat.st_ctime, stat.st_mtime)

        self._server_cache = set()
        with open(NTP.CFG_PATH, "r") as cfg:
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
        with open(NTP.CFG_PATH, "r+") as cfg:
            newcfg = ["### Switch Light\n"]
            for line in cfg.readlines():
                if line.startswith("#") or not line.strip():
                    continue
                newcfg.append(line)
            cfg.seek(0)
            cfg.truncate()
            cfg.write("".join(newcfg))

    def _write_cache (self):
        newcfg = []
        with open(NTP.CFG_PATH, "rw+") as cfg:
            for line in cfg.readlines():
                if line.startswith("server"):
                    continue
                newcfg.append(line)

            # MUST GRAB THE INTERNAL CACHE!
            # We've whacked it by this point, so ctime/mtime will have changed
            for server in self._server_cache:
                newcfg.append("server %s iburst\n" % (server))
            cfg.seek(0)
            cfg.truncate()
            cfg.write("".join(newcfg))
            
    def _remove_server (self, server, is_init):
        ### FIXME: Need a lock
        if server not in self.servers:
            raise UnknownServerError(server)
        self.servers.remove(server)
        self._write_cache()
        if NTP.status() == Service.RUNNING:
            NTP.restart(deferred=is_init)

    def _add_server (self, server, is_init):
        if server in self.servers:
            raise KnownServerError(server)
        self.servers.add(server)
        self._write_cache()
        if NTP.status() == Service.RUNNING:
            NTP.restart(deferred=is_init)
        

    def cli_config (self, no_command, data, is_init):
        if no_command:
            try:
                self._remove_server(data["server"], is_init)
            except UnknownServerError, e:
                print "Server '%s' is not configured" % (data["server"])
        else:
            try:
                self._add_server(data["server"], is_init)
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

    def cli_sync (self, data):
        if NTP.status() == Service.RUNNING:
            try:
                NTP.disable()
                #allow large time difference, set clock and exit,
                #run in foreground
                try:
                    shell.call('ntpd -g -q -n', timeout=10)
                except subprocess.CalledProcessError, e:
                    if e.returncode == -signal.SIGKILL:
                        print "ntp sync timed out"
                    else: 
                        print "ntp sync failed, rc: %d" % e.returncode
                NTP.enable()
            except:
                pass
        else:
            print "NTP disabled"

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
            shell.call('ntpq -p', show_output=True)
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
                        'is_init'    : '$is-init',
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
                        'type'            : 'ip-address-or-domain-name',
                        'optional-for-no' : False,
                        'syntax-help'     : 'IP address or domain name of NTP Server',
                    },
                ),
            ),
        },
    ),
}

command.add_action('implement-sync-ntp', NTPConfig.cli_sync,
                    {'kwargs' : {
                        'data'       : '$data',
                    } } )

SYNC_NTP_COMMAND_DESCRIPTION = {
    'name'         : 'ntp',
    'mode'         : 'enable',
    'action'       : 'implement-sync-ntp',
    'no-supported' : False,
    'args'         : (
        {
            'token'       : 'sync',
            'short-help'  : 'Synchronize clock via NTP',
            'doc'         : 'clock|ntp-sync',
        },
    )
}
