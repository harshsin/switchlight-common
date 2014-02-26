# Copyright (c) 2013  BigSwitch Networks

import os

import command
import run_config

from sl_util import Service

class RSyslog(Service):
    SVC_NAME = "rsyslog"

class _LoggingConfig(object):
    RPATH = "/etc/rsyslog.d/10-switchlight-remote.conf"

    def __init__ (self):
        pass

    @property
    def remotes (self):
        remotes = []
        try:
            with open(_LoggingConfig.RPATH, "r") as cfg:
                for line in cfg:
                    remotes.append(self.parseHost(line.strip()))
        except IOError:
            pass
        return set(remotes)

    @remotes.setter
    def remotes (self, vals):
        old_remotes = self.remotes
        vals = set(vals)
        if vals == old_remotes:
            return
        with open(_LoggingConfig.RPATH, "w+") as cfg:
            for remote in vals:
                cfg.write("%s\n" % (remote.conf_entry))
        
    def parseHost (self, line):
        for typ in REMOTETYPES:
            try:
                return typ(line)
            except UnsupportedTransportError:
                continue

    def cli_config (self, no_command, data, init):
        existing = self.remotes
        if data.has_key('tcp'):
            remote = TCPRemote()
        else:
            remote = UDPRemote()
        remote.host = data['host']
        if no_command:
            try:
                existing.remove(remote)
            except KeyError:
                print "No match"
            self.remotes = existing
        else:
            existing.add(remote)
            self.remotes = existing

        RSyslog.restart(deferred=init)

    def cli_show_running (self, context, runcfg, words):
        comp_runcfg = []

        for remote in self.remotes:
            comp_runcfg.append(remote.runcfg)

        # attach component-specific config
        if comp_runcfg:
            runcfg.append('!\n')
            runcfg += ["%s\n" % (line) for line in comp_runcfg]
        
class UnsupportedTransportError(Exception):
    def __init__ (self, val):
        self.val = val
    def __str__ (self):
        return "UnsupportedTransportError: %s" % (self.val)
        
class Remote(object):
    def __init__ (self):
        self._host = None
        self.match = "*.*"

    @property
    def host (self):
        return self._host

    @host.setter
    def host (self, val):
        self._host = val

    def __eq__ (self, other):
        if type(other) == type(self):
            return self.host == other.host and self.match == other.match
        else:
            return False

    def __str__ (self):
        return "<%s> %s: %s" % (type(self), self.host, self.match)

    def __hash__ (self):
        return hash(str(self))

    def __lt__ (self, other):
        return str(self) < str(other)
                
class UDPRemote(Remote):
    def __init__ (self, line = None):
        super(UDPRemote, self).__init__()
        if line:
            fields = line.split()
            if fields[1].startswith("@@"):
                raise UnsupportedTransportError(fields[1])
            elif fields[1].startswith("@"):
                self.host = fields[1][1:]
                self.match = fields[0]

    @property
    def conf_entry (self):
        return "%s\t@%s" % (self.match, self.host)

    @property
    def runcfg (self):
        return "logging host %s" % (self.host)

class TCPRemote(Remote):
    def __init__ (self, line = None):
        super(TCPRemote, self).__init__()
        if line:
            fields = line.split()
            if fields[1].startswith("@@"):
                self.host = fields[1][2:]
                self.match = fields[0]
            else:
                raise UnsupportedTransportError(fields[1])

    @property
    def conf_entry (self):
        return "%s\t@@%s" % (self.match, self.host)

    @property
    def runcfg (self):
        return "logging host %s tcp" % (self.host)

REMOTETYPES = [UDPRemote, TCPRemote]

LoggingConfig = _LoggingConfig()


command.add_action('implement-config-logging', LoggingConfig.cli_config,
                    {'kwargs': {
                                 'no_command' : '$is-no-command',
                                 'data'       : '$data',
                                 'init'       : '$is-init',
                               } } )

# FIXME can more than one external syslog server be specified?
CONFIG_LOGGING_COMMAND_DESCRIPTION = {
    'name'         : 'logging',
    'mode'         : 'config',
    'short-help'   : 'Configure logging parameters',
    'action'       : 'implement-config-logging',
    'no-action'    : 'implement-config-logging',
    'doc'          : 'rlog|logging',
    'doc-example'  : 'rlog|logging-example',
    'args'         : (
        {
            'choices' : (
                (
                    {
                        'token'           : 'host',
                        'doc'             : 'rlog|host',
                    },
                    {
                        'field'           : 'host',
                        'type'            : 'ip-address-or-domain-name',
                        'syntax-help'     : 'External syslog server hostname or address'
                    },
                    {
                        'token'           : 'tcp',
                        'data'            : { 'tcp'  : True },
                        'optional'        : True,
                        'optional-for-no' : True,
                        'short-help'      : 'Log using TCP instead of UDP',
                        'doc'             : 'rlog|host-tcp',
                    },
                ),
            ),
        },
    ),
}

logging_running_config_tuple = (
    (
        {
            'optional'   : False,
            'field'      : 'running-config',
            'type'       : 'enum',
            'values'     : 'logging',
            'short-help' : 'Configuration for logging',
            'doc'        : 'running-config|show-logging',
        },
    ),
)

run_config.register_running_config('logging', 2500,  None,
                                   LoggingConfig.cli_show_running,
                                   logging_running_config_tuple)

