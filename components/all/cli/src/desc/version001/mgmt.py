# Copyright (c) 2013  BigSwitch Networks

import socket
import fcntl
import struct
import array
import os
import os.path
import subprocess
import sys

from sl_util import shell
from sl_util import const

import command
import run_config

import utif
import error

DHCLIENT_CFG = """### SwitchLight

option rfc3442-classless-static-routes code 121 = array of unsigned integer 8;

request subnet-mask, broadcast-address, time-offset, routers,
	domain-name, domain-name-servers, domain-search, host-name,
	netbios-name-servers, netbios-scope, interface-mtu,
	rfc3442-classless-static-routes, ntp-servers,
	dhcp6.domain-search, dhcp6.fqdn,
	dhcp6.name-servers, dhcp6.sntp-servers, vendor-encapsulate-options;
"""

SL_DHCP_EXIT_HOOK = """### SwitchLight
RUN="yes"

if [ "$RUN" = "yes" ]; then
    cd /usr/lib/python*/dist-packages/pcli
    exec python sl-dhclient-cfg.py "$@"
fi
"""

def show_single_intf(ifname):
    try:
        intf = NetworkConfig.getInterface(ifname)
        print "%s is %s" % (intf.name, intf.operstate)
        print "  Hardware Address: %s" % (intf.dladdr)
        print "  IPv4 Address(es): %s" % (", ".join(intf.v4addrs))
        try:
            print "  MTU %d bytes, Speed %s Mbps" % (intf.mtu, intf.speed)
            stats = intf.stats
            print "  Received: %(rx-bytes)d bytes, %(rx-packets)d packets" % (stats)
            print "            %(rx-errors)d errors, %(rx-drops)d drops" % (stats)
            print "  Sent:     %(tx-bytes)d bytes, %(tx-packets)d packets" % (stats)
            print "            %(tx-errors)d errors, %(tx-drops)d drops" % (stats)
        except InterfaceDownError, e:
            return
    except UnknownInterfaceError, e:
        raise error.ActionError('Bad interface %s' % ifname)

def show_mgmt_intf(data):
    if 'ifname' in data:
        show_single_intf(data['ifname'])
    else:
        for ifname in const.MGMT_PORTS:
            show_single_intf(ifname)


class InterfaceDownError(Exception):
    def __init__ (self, name):
        self.name = name
    def __str__ (self):
        return "InterfaceDownError: [%s] Attempt to access attribute which requires active interface" % (name)

class InterfaceStateError(Exception):
    def __init__ (self, state):
        self.state = state
    def __str__ (self):
        return "InterfaceStateError: State [%s] is unsupported" % (self.state)

class Interface(object):
    START_DHCLIENT = "dhclient -4 -e IF_METRIC=100 -pf /var/run/dhclient.%(name)s.pid -lf /var/lib/dhcp/dhclient.%(name)s.leases -1 %(name)s"
    STOP_DHCLIENT = "dhclient -r -pf /var/run/dhclient.%(name)s.pid"
    STOP_DHCLIENT_PERSIST = "dhclient -x -pf /var/run/dhclient.%(name)s.pid"

    STAT_FIELDS = ("rx-bytes", "rx-packets", "rx-errors", "rx-drops", "rx-fifo",
                   "rx-frame", "rx-compressed", "rx-multicast",
                   "tx-bytes", "tx-packets", "tx-errors", "tx-drops", "tx-fifo",
                   "tx-collisions", "tx-carrier", "tx-compressed")
    UP = "up"
    DOWN = "down"

    def __init__ (self, name):
        self.name = name

    def _getSysAttr (self, attr):
        try:
            with open("/sys/class/net/%s/%s" % (self.name, attr), "r") as data:
                return data.read().strip()
        except IOError, e:
            if e.errno == 22:
                raise InterfaceDownError(self.name)
            else:
                raise

    @property
    def operstate (self):
        return self._getSysAttr("operstate")

    @operstate.setter
    def operstate (self, val):
        if val == self.operstate:
            return
        if val == Interface.UP:
            shell.call("ip link set %s up" % (self.name))
        elif val == Interface.DOWN:
            shell.call("ip link set %s down" % (self.name))
        else:
            raise InterfaceStateError(val)

    @property
    def mtu (self):
        return int(self._getSysAttr("mtu"))

    @mtu.setter
    def mtu (self, val):
        shell.call("ip link set %s mtu %d" % (self.name, val))

    @property
    def speed (self):
        return int(self._getSysAttr("speed"))

    @property
    def dladdr (self):
        return self._getSysAttr("address")

    @property
    def dhcp_enabled (self):
        return os.path.exists("/etc/sl-dhcp-enabled-%s" % (self.name))

    @dhcp_enabled.setter
    def dhcp_enabled (self, enabled):
        if enabled == self.dhcp_enabled:
            return
        if enabled:
            shell.call("touch /etc/sl-dhcp-enabled-%s" % (self.name))
            shell.call(Interface.START_DHCLIENT % ({"name" : self.name}))
        else:
            shell.call(Interface.STOP_DHCLIENT % ({"name" : self.name}))
            shell.call("rm /etc/sl-dhcp-enabled-%s" % (self.name))

    @property
    def dhcp_sticky (self):
        DHCLIENT_PID_PATH = "/var/run/dhclient.%s.pid" % (self.name)
        if self.dhcp_enabled:
            if not os.path.exists(DHCLIENT_PID_PATH):
                return True
        return False

    @dhcp_sticky.setter
    def dhcp_sticky (self, val):
        if val:
            # Kill dhclient
            DHCLIENT_PID_PATH = "/var/run/dhclient.%s.pid" % (self.name)
            if os.path.exists(DHCLIENT_PID_PATH):
                with open(DHCLIENT_PID_PATH, "r") as pf:
                    pid = pf.read().strip()
                    shell.call("kill -9 %s" % (pid))
                os.unlink(DHCLIENT_PID_PATH)
        else:
            # Turn dhclient back on
            self.dhcp_enabled = True

    @property
    def v4addrs (self):
        (sout, serr, rc) = shell.call("ip addr show %s" % (self.name))
        return [line.split()[1] for line in sout.split("\n") if line.strip().startswith('inet ')]

    @v4addrs.setter
    def v4addrs (self, new):
        new = set(new)
        old = set(self.v4addrs)
        to_remove = old - new
        to_add = new - old
        for net in to_remove:
            shell.call("ip addr del %s dev %s" % (net, self.name))
        for net in to_add:
            shell.call("ip addr add %s dev %s" % (net, self.name))

    @property
    def stats (self):
        with open("/proc/net/dev", "r") as data:
            for line in data:
                if line.strip().startswith(self.name):
                    sdata = line.strip().split()
                    data = [int(x) for x in sdata[1:]]
                    return dict(zip(Interface.STAT_FIELDS, data))
          
class UnknownInterfaceError(Exception):
    def __init__ (self, name):
        self.name = name
    def __str__ (self):
        return "UnknownInterfaceError: [%s]" % (self.name)

class _NetworkConfig(object):
    def __init__ (self):
        self._setupDHClient()
        self._interfaces = {}
        self._populateInterfaces()

    def _setupDHClient (self):
        # This is preposterously heavyhanded
        with open("/etc/dhcp/dhclient.conf", "w+") as dhcfg:
            dhcfg.write(DHCLIENT_CFG)
        with open("/etc/dhcp/dhclient-exit-hooks.d/switchlight", "w+") as sl:
            sl.write(SL_DHCP_EXIT_HOOK)

    def _populateInterfaces (self):
        names = os.listdir("/sys/class/net/")
        for name in names:
            self._interfaces[name] = Interface(name)

    def getInterface (self, name):
        try:
            return self._interfaces[name]
        except KeyError, e:
            raise UnknownInterfaceError(name)

    @property
    def default_gateways (self):
        gw = []
        with open("/proc/net/route", "r") as routes:
            for line in routes:
                if line.strip():
                    fields = line.strip().split()
                    if fields[1] == "Destination":
                        continue
                    if int(fields[1], 16) == 0:
                        gw.append(socket.inet_ntoa(struct.pack('i', int(fields[2], 16))))
        return gw

    def configGateway (self, no_command, data):
        gws = set(self.default_gateways)
        if no_command:
            if data["gw"] in gws:
                shell.call("route del default gw %s" % (data["gw"]))
        else:
            shell.call("route add default gw %s" % (data["gw"]))
        
    
NetworkConfig = _NetworkConfig()

command.add_action('implement-show-mgmt-intf', show_mgmt_intf,
                   {'kwargs': { 'data'      : '$data', } } )


# hardcoded for ipv4
def config_mgmt_intf(no_command, data):
    if 'shutdown' in data:
        raise error.ActionError('Shutdown not available for ' + data['ifname'])

    try:
        intf = NetworkConfig.getInterface(data['ifname'])
        addrs = set(intf.v4addrs)
        if no_command:
            if data.has_key("dhcp"):
                if intf.dhcp_enabled:
                    intf.dhcp_enabled = False
                else:
                    raise error.ActionError("DHCP is not enabled for %s" %
                                            data['ifname'])
            elif data.has_key("ip-address"):
                if data["ip-address"] in addrs:
                    addrs.remove(data["ip-address"])
                intf.v4addrs = addrs
                return
        else:
            if data.has_key("dhcp"):
                intf.operstate = Interface.UP
                intf.dhcp_enabled = True
                if data.has_key("sticky"):
                    intf.dhcp_sticky = True
            elif data.has_key("ip-address"):
                intf.operstate = Interface.UP
                # allow only one IPv4 address for the interface
                intf.v4addrs = [ data["ip-address"] ]
                return
    except UnknownInterfaceError, e:
        raise error.ActionError('Bad interface %s' % data['ifname'])

command.add_action('implement-config-mgmt-intf', config_mgmt_intf,
                    {'kwargs': {
                                 'no_command' : '$is-no-command',
                                 'data'       : '$data',
                               } } )


class UnknownServerError(Exception):
    def __init__(self, server):
        self.server = server

class KnownServerError(Exception):
    def __init__(self, server):
        self.server = server

class _DNSConfig(object):
    PATH = "/etc/resolv.conf"

    def __init__(self):
        self._domain_cache = ''
        self._server_cache = []
        self._cache_tinfo = (0,0)

        firstboot = False
        with open(_DNSConfig.PATH, "r") as cfg:
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
    def servers(self):
        stat = os.stat(_DNSConfig.PATH)
        if ((stat.st_ctime != self._cache_tinfo[0]) or
            (stat.st_mtime != self._cache_tinfo[1])):
            self._rebuild_cache()
        return self._server_cache
        
    @property
    def domain(self):
        stat = os.stat(_DNSConfig.PATH)
        if ((stat.st_ctime != self._cache_tinfo[0]) or
            (stat.st_mtime != self._cache_tinfo[1])):
            self._rebuild_cache()
        return self._domain_cache

    @domain.setter
    def domain(self, val):
        self._domain_cache = val
        self._write_cache()
        
    def _rebuild_cache(self):
        ### FIXME: Log this
        stat = os.stat(_DNSConfig.PATH)
        self._cache_tinfo = (stat.st_ctime, stat.st_mtime)

        self._domain_cache = ''
        self._server_cache = []
        with open(_DNSConfig.PATH, "r") as cfg:
            for line in cfg.readlines():
                if line.startswith("nameserver"):
                    d = line.split()
                    if len(d) >= 2:
                        self._server_cache.append(d[1])
                    else:
                        ### FIXME: Log something
                        continue
                if line.startswith("domain"):
                    d = line.split()
                    if len(d) >= 2:
                        self._domain_cache = d[1]
                    else:
                        ### FIXME: Log something
                        continue

    def _firstboot(self):
        with open(_DNSConfig.PATH, "r+") as cfg:
            newcfg = ["### Switch Light\n"]
            for line in cfg.readlines():
                if line.startswith("#") or not line.strip():
                    continue
                newcfg.append(line)
            cfg.seek(0)
            cfg.truncate()
            cfg.write("".join(newcfg))

    def _write_cache(self):
        with open(_DNSConfig.PATH, "w+") as cfg:
            cl = ["### Switch Light\n"]
            # MUST GRAB THE INTERNAL CACHE!
            # We've whacked it by this point, so ctime/mtime will have changed
            if self._domain_cache != '':
                cl.append("domain %s\n" % self._domain_cache)
            for server in self._server_cache:
                cl.append("nameserver %s\n" % (server))
            cfg.write("".join(cl))
            
    def _remove_server(self, server):
        ### FIXME: Need a lock
        if server not in self.servers:
            raise UnknownServerError(server)
        self.servers.remove(server)
        self._write_cache()

    def _add_server(self, server):
        if server in self.servers:
            raise KnownServerError(server)
        self.servers.append(server)
        self._write_cache()

    def cli_config (self, no_command, data):
        if 'server' in data:
            if no_command:
                # FIXME handle 'no dns-server'
                try:
                    self._remove_server(data["server"])
                except UnknownServerError, e:
                    print "Server '%s' is not configured" % (data["server"])
            else:
                try:
                    self._add_server(data["server"])
                except KnownServerError, e:
                    print "Server '%s' is already configured" % (data["server"])
        if 'domain' in data:
            if no_command:
                if 'domain' in data and data['domain'] == self._domain_cache:
                    self.domain = ''
                else:
                    raise error.ActionError('Unknown domain %s' % 
                                            data['domain'])
            else:
                self.domain = data['domain']

    def cli_running_config (self, comp_runcfg):
        if self._domain_cache != '':
            comp_runcfg.append("dns-domain %s\n" % self._domain_cache)
        for server in self.servers:
            comp_runcfg.append("dns-server %s\n" % server)


DNSConfig = _DNSConfig()

command.add_action('implement-config-dns', DNSConfig.cli_config,
                    {'kwargs': { 'no_command' : '$is-no-command',
                                 'data'       : '$data', } } )

DNS_SERVER_COMMAND_DESCRIPTION = {
    'name'         : 'dns-server',
    'mode'         : 'config',
    'short-help'   : 'Configure DNS servers',
    'action'       : 'implement-config-dns',
    'no-action'    : 'implement-config-dns',
    'doc'          : 'mgmt|dns-server',
    'doc-example'  : 'mgmt|dns-server-example',
    'args'         : (
        {
            'field'           : 'server',
            'type'            : 'ip-address',
            'optional'        : False,
            'optional-for-no' : True,
            'completion-text' : 'ip address',
            'syntax-help'     : 'IP address of DNS server',
            'doc'             : 'mgmt|dns-server-address'
        },
    ),
}

DNS_DOMAIN_COMMAND_DESCRIPTION = {
    'name'         : 'dns-domain',
    'mode'         : 'config',
    'short-help'   : 'Configure DNS domain',
    'action'       : 'implement-config-dns',
    'no-action'    : 'implement-config-dns',
    'doc'          : 'mgmt|dns-domain',
    'doc-example'  : 'mgmt|dns-domain-example',
    'args'         : (
        {
            'field'           : 'domain',
            'type'            : 'domain-name',
            'optional'        : False,
            'optional-for-no' : True,
            'completion-text' : 'DNS domain',
            'syntax-help'     : 'DNS domain name',
            'doc'             : 'mgmt|dns-domain-name'
        },
    ),
}

command.add_action('implement-config-gateway', NetworkConfig.configGateway,
                    {'kwargs' : {'data' : '$data',
                                 'no_command' : '$is-no-command',}})

IP_DEFAULT_GATEWAY_COMMAND_DESCRIPTION = {
    'name'          : 'ip',
    'mode'          : 'config',
    'short-help'    : 'Configure routing table',
    'action'        : 'implement-config-gateway',
    'no-action'     : 'implement-config-gateway',
    'doc'           : 'mgmt|ip',
    'doc-example'   : 'mgmt|ip-example',
    'args'          : (
        {
            'token'         : 'default-gateway',
            'short-help'    : 'Set default gateway',
            'doc'           : 'mgmt|ip-default-gateway',
        },
        {
            'field'         : 'gw',
            'type'          : 'ip-address',
            'syntax-help'   : 'IP address of default gateway',
        },
    ),
}

def running_config_interface(context, runcfg, words):
    comp_runcfg = []

    # collect component-specific config
    for ifname in const.MGMT_PORTS:
        intf = NetworkConfig.getInterface(ifname)

        if intf.dhcp_enabled:
            line = "interface %s ip-address dhcp" % (ifname)
            if intf.dhcp_sticky:
                line += " sticky"
            comp_runcfg.append("%s\n" % (line))
        else:
            for addr in intf.v4addrs:
                comp_runcfg.append('interface %s ip-address %s\n' % 
                                   (ifname, addr))
            for gw in NetworkConfig.default_gateways:
                comp_runcfg.append("ip default-gateway %s\n" % (gw))
            DNSConfig.cli_running_config(comp_runcfg)

    # attach component-specific config
    if len(comp_runcfg) > 0:
        runcfg.append('!\n')
        runcfg += comp_runcfg

interface_running_config_tuple = (
    (
        {
            'optional'   : False,
            'field'      : 'running-config',
            'type'       : 'enum',
            'values'     : 'interface',
            'short-help' : 'Configuration for interfaces',
            'doc'        : 'running-config|show-interface',
        },
    ),
)

run_config.register_running_config('interface', 1000,  None,
                                   running_config_interface,
                                   interface_running_config_tuple)

