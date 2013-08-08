# Copyright (c) 2013  BigSwitch Networks

import os.path
import urlparse

import command
import run_config
import error


class _BootConfig(object):
    PATH = "/mnt/flash/boot-config"
    NET_SCHEMES = set(['http', 'ftp', 'tftp',
                    'nfs', 'ssh', 'scp'])
    LOCAL_SCHEMES = set(os.listdir("/mnt"))

    def __init__ (self):
        pass

    def write_attribute (self, attr, val, do_delete):
        bl = []
        seen = False
        with open(_BootConfig.PATH, "a+") as cfg:
            cfg.seek(0)
            for line in cfg.readlines():
                if line.startswith("%s=" % (attr)):
                    if do_delete:
                        if val != '' and val != line.split('=')[1].strip():
                            raise error.ActionError('Does not match configured value')
                        # delete key/value pair by not adding to bl
                    else:
                        bl.append("%s=%s\n" % (attr, val))
                        seen = True
                else:
                    bl.append(line)
            if not seen and not do_delete:
                bl.append("%s=%s\n" % (attr, val))
            cfg.seek(0)
            cfg.truncate()
            cfg.write("".join(bl))

    def read_all (self):
        if os.path.exists(_BootConfig.PATH):
            with open(_BootConfig.PATH, "r") as cfg:
                return cfg.readlines()
        else:
            return []

    def cli_show (self, data):
        print "".join(self.read_all())

    def cli_config (self, no_command, data):
        for key in data:
            if key == 'image':
                if data['image']:
                    pr = urlparse.urlparse(data['image'])
                    if pr.scheme == "":
                        raise error.ArgumentValidationError( \
                            "Bad image string '%s'" % (data['image']))
                    if ((pr.scheme not in _BootConfig.NET_SCHEMES) and
                        (pr.scheme not in _BootConfig.LOCAL_SCHEMES)):
                        raise error.CommandSemanticError( \
                            "Unsupported protocol '%s'" % (pr.scheme))
                self.write_attribute("SWI", data['image'], no_command)
            else:
                self.write_attribute(key.upper(), data[key], no_command)


BootConfig = _BootConfig()

command.add_action('implement-show-boot', BootConfig.cli_show,
                    {'kwargs': {'data'      : '$data',}})

SHOW_BOOT_COMMAND_DESCRIPTION = {
    'name'         : 'show',
    'mode'         : 'login',
    'action'       : 'implement-show-boot',
    'no-supported' : False,
    'args'         : (
        {
            'token'       : 'boot',
            'short-help'  : 'Show switch boot information',
            'doc'         : 'boot|show-boot',
        },
    )
}

command.add_action('implement-boot-config', BootConfig.cli_config,
                    {'kwargs': {
                                 'no_command' : '$is-no-command',
                                 'data'       : '$data',
                               } } )

CONFIG_BOOT_COMMAND_DESCRIPTION = {
    'name'         : 'boot',
    'mode'         : 'config',
    'short-help'   : 'Set boot configuration parameters',
    'action'       : 'implement-boot-config',
    'no-action'    : 'implement-boot-config',
    'doc'          : 'boot|boot',
    'doc-example'  : 'boot|boot-example',
    'args'         : (
        {
            'choices': (
                (
                    {
                        'token'       : 'image',
                        'short-help'  : 'Set boot image',
                        'doc'         : 'boot|boot-image',
                        'data'        : { 'image' : '' },
                    },
                    {
                        'field'       : 'image',
                        'type'        : 'string',
                        'optional-for-no' : True,
                        'syntax-help' : 'Image URL',
                    },
                ),
                (
                    {
                        'token'       : 'netdev',
                        'short-help'  : 'Interface for loading image or configuration',
                        'doc'         : 'boot|boot-netdev',
                        'data'        : { 'netdev' : '' },
                    },
                    {
                        'field'       : 'netdev',
                        'type'        : 'enum',
                        'values'      : ('ma1'),
                        'optional-for-no' : True,
                        'syntax-help' : 'Management interface',
                    },
                ),
                (
                    {
                        'token'       : 'netauto',
                        'short-help'  : 'Automatic network interface configuration method',
                        'doc'         : 'boot|boot-netauto',
                        'data'        : { 'netauto' : '' },
                    },
                    {
                        'field'       : 'netauto',
                        'type'        : 'enum',
                        'values'      : ('dhcp'),
                        'optional-for-no' : True,
                        'syntax-help' : 'Network interface configuration method',
                    },
                ),
                (
                    {
                        'token'       : 'netip',
                        'short-help'  : 'IP address for netdev interface',
                        'doc'         : 'boot|boot-netip',
                        'data'        : { 'netip' : '' },
                    },
                    {
                        'field'       : 'netip',
                        'type'        : 'ip-address',
                        'optional-for-no' : True,
                        'syntax-help' : 'IP address for netdev interface',
                    },
                ),
                (
                    {
                        'token'       : 'netmask',
                        'short-help'  : 'Netmask for netdev interface',
                        'doc'         : 'boot|boot-netmask',
                        'data'        : { 'netmask' : '' },
                    },
                    {
                        'field'       : 'netmask',
                        'type'        : 'netmask',
                        'optional-for-no' : True,
                        'syntax-help' : 'Netmask for netdev interface',
                    },
                ),
                (
                    {
                        'token'       : 'netgw',
                        'short-help'  : 'Gateway for netdev interface',
                        'doc'         : 'boot|boot-netgw',
                        'data'        : { 'netgw' : '' },
                    },
                    {
                        'field'       : 'netgw',
                        'type'        : 'ip-address',
                        'optional-for-no' : True,
                        'syntax-help' : 'Gateway for netdev interface',
                    },
                ),
                (
                    {
                        'token'       : 'netdomain',
                        'short-help'  : 'DNS domain name for device',
                        'doc'         : 'boot|boot-netdomain',
                        'data'        : { 'netdomain' : '' },
                    },
                    {
                        'field'       : 'netdomain',
                        'type'        : 'domain-name',
                        'optional-for-no' : True,
                        'syntax-help' : 'DNS domain name for device',
                    },
                ),
                (
                    {
                        'token'       : 'netdns',
                        'short-help'  : 'IP address of DNS server',
                        'doc'         : 'boot|boot-netdns',
                        'data'        : { 'netdns' : '' },
                    },
                    {
                        'field'       : 'netdns',
                        'type'        : 'ip-address',
                        'optional-for-no' : True,
                        'syntax-help' : 'IP address of DNS server',
                    },
                ),
            ),
        },
    ),
}

