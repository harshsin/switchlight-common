# Copyright (c) 2013  BigSwitch Networks

import command
import run_config
import error

import subprocess
from sl_util import shell
from sl_util import OFConnection
from sl_util.ofad import OFADConfig

import loxi.of10 as of10

import re
import itertools

OFAgentConfig = OFADConfig()

def get_base_name(s):
    # given a string, return the base name of an interface; 
    # FIXME assumes it's all text...
    match = re.match(r'([A-Za-z])+', s)
    return match.group() if match else ''

def get_port_name(s):
    # pyloxi returns the entire field, 
    # but we only want the part of the string before the null
    return s.split('\x00')[0]

def get_port_info():
    # returns a dict of [port_desc, port_stats], keyed by port name

    conn = OFConnection.OFConnection('127.0.0.1', 6634)

    # store port_desc from features_reply
    ports = {}
    num2name = {}
    features_reply = conn.request_features()
    for p in features_reply.ports:
        p.name = get_port_name(p.name)
        ports[get_port_name(p.name)] = [p]
        num2name[p.port_no] = get_port_name(p.name)

    # merge in rx and tx packet counts
    for pse in conn.of10_request_stats(of10.message.port_stats_request( \
            port_no=of10.OFPP_ALL)):
        ports[num2name[pse.port_no]].append(pse)

    conn.close()

    return ports


class Platform(object):
    def __init__(self):
        self._dp_intf_base = None
        self._dp_intf_min = None
        self._dp_intf_max = None
        self._lag_intf_base = None
        self._lag_intf_min = None
        self._lag_intf_max = None
        self._mgmt_intf_base = 'ma'
        self._mgmt_intf_min = 1
        self._mgmt_intf_max = 1

    def populate(self):
        self._dp_intf_base = OFAgentConfig.physical_base_name
        self._lag_intf_base = OFAgentConfig.lag_base_name

        # FIXME we populate the dataplane-related info once;
        # this needs to be revisited if this info can change
        ports = get_port_info()

        dp_port_nums = []
        lag_port_nums = []
        for p in ports.keys():
            if p.startswith(self._dp_intf_base):
                dp_port_nums.append(int(p[len(self._dp_intf_base):]))
            elif p.startswith(self._lag_intf_base):
                lag_port_nums.append(int(p[len(self._lag_intf_base):]))

        if dp_port_nums:
            self._dp_intf_min = min(dp_port_nums)
            self._dp_intf_max = max(dp_port_nums)

        if lag_port_nums:
            self._lag_intf_min = min(lag_port_nums)
            self._lag_intf_max = max(lag_port_nums)

    @property
    def dp_intf_base(self):
        if self._dp_intf_base is None:
            self.populate()
        return self._dp_intf_base

    @property
    def dp_intf_min(self):
        if self._dp_intf_base is None:
            self.populate()
        return self._dp_intf_min

    @property
    def dp_intf_max(self):
        if self._dp_intf_base is None:
            self.populate()
        return self._dp_intf_max

    @property
    def lag_intf_base(self):
        if self._lag_intf_base is None:
            self.populate()
        return self._lag_intf_base

    @property
    def lag_intf_min(self):
        if self._lag_intf_base is None:
            self.populate()
        return self._lag_intf_min

    @property
    def lag_intf_max(self):
        if self._lag_intf_base is None:
            self.populate()
        return self._lag_intf_max

    @property
    def mgmt_intf_base(self):
        return self._mgmt_intf_base

    @property
    def mgmt_intf_min(self):
        return self._mgmt_intf_min

    @property
    def mgmt_intf_max(self):
        return self._mgmt_intf_max

plat = Platform()


def is_err(val):
    return val != 0xffffffffffffffff and val > 0
def display(val):
    return str(val) if val != 0xffffffffffffffff else '---'
def get_speed(bmap):
    if bmap & of10.OFPPF_10GB_FD:
        return '10G'
    elif bmap & (of10.OFPPF_1GB_FD | of10.OFPPF_1GB_HD):
        return '1G'
    elif bmap & (of10.OFPPF_100MB_FD | of10.OFPPF_100MB_HD):
        return '100M'
    elif bmap & (of10.OFPPF_10MB_FD | of10.OFPPF_10MB_HD):
        return '10M'
    else:
        return ''

def show_one_dp_intf_detail(port):
    # prints detailed info for the given (port_desc, port_stats) tuple
    print '%s is %s' % \
        (port[0].name, 
         'down' if port[0].state & of10.OFPPS_LINK_DOWN
         else 'admin down' if port[0].config & of10.OFPPC_PORT_DOWN
         else 'up')
    print '  Hardware Address: %s' % of10.util.pretty_mac(port[0].hw_addr)
    print '  Speed: %s' % get_speed(port[0].curr)
    print '  Received: %s bytes, %s packets' % \
        (display(port[1].rx_bytes), display(port[1].rx_packets))
    print '            %s errors, %s drops' % \
        (display(port[1].rx_errors), display(port[1].rx_dropped))
    print '            %s frame, %s overruns, %s crc' % \
        (display(port[1].rx_frame_err),
         display(port[1].rx_over_err), display(port[1].rx_crc_err))
    print '  Sent:     %s bytes, %s packets' % \
        (display(port[1].tx_bytes), display(port[1].tx_packets))
    print '            %s errors, %s drops' % \
        (display(port[1].tx_errors), display(port[1].tx_dropped))
    print '            %s collisions' % display(port[1].collisions)

def show_one_dp_intf_summary(format_str, port):
    # prints summary info for the given (port_desc, port_stats) tuple

    if port[0].config & of10.OFPPC_PORT_DOWN:
        state = 'D'
    elif (port[0].state & of10.OFPPS_LINK_DOWN) == 0:
        state = '*'
    else:
        state = ' '
    if is_err(port[1].rx_errors) or is_err(port[1].tx_errors) or \
            is_err(port[1].rx_frame_err) or is_err(port[1].rx_over_err) or \
            is_err(port[1].rx_crc_err):
        err = '+Errors'
    else:
        err = '      '
    print format_str % \
        (str(port[0].port_no), state, 
         get_port_name(port[0].name), get_speed(port[0].curr),
         port[1].rx_packets, port[1].tx_packets, err)

def show_dp_intf_list(port_name_list, detail):
    ports = get_port_info()

    if not detail:
        format_str = "%2s%s %-12s %-5s %20s %20s %7s"
        print '* = Link up, D = Disabled'
        print format_str % ('#', ' ', 'Name', 'Speed', 'Rx', 'Tx', '  ')

    if port_name_list:
        filt = lambda p: get_port_name(p[0].name) in port_name_list
    else:
        filt = lambda p: True

    for p in itertools.ifilter(filt,
                               iter(sorted(ports.itervalues(), \
                                               key=lambda p: p[0].port_no))):
        if not detail:
            show_one_dp_intf_summary(format_str, p)
        else:
            show_one_dp_intf_detail(p)

def parse_port_list(port_list, orig_base, new_base, minport, maxport):
    # parse a port list, returning a list of the names of all specified ports.
    # first strips the string 'orig_base' from the port_list, 
    # generates the port list, and then prepends new_base to all ports.
    # ports are checked to be in the range minport to maxport, inclusive.
    # port_list := range | [, range ]
    # range := port | low_port-high_port
    # assumes input only contains [,-0-9]

    ports = []

    # slicing strips orig_base from port_list
    for rng in port_list[len(orig_base):].split(','):
        vals = rng.split('-')
        if len(vals) == 1 and vals[0]:
            port = int(vals[0])
            if port < minport or port > maxport:
                raise error.ActionError('Bad port ' + rng)
            if port not in ports:
                ports.append(port)
        elif len(vals) == 2:
            lo = int(vals[0]) if vals[0] else minport
            hi = int(vals[1]) if vals[1] else maxport
            if lo < minport or hi > maxport or lo > hi:
                raise error.ActionError('Bad range ' + rng)
            else:
                for i in range(lo, hi+1):
                    if i not in ports:
                        ports.append(i)
        else:
            raise error.ActionError('Bad range ' + rng)

    # prepend new_base to each port number
    return [ new_base + str(pnum) for pnum in sorted(ports) ]


def show_intf(data):
    if 'intf-port-list' in data:
        # extract the interface base name
        base = get_base_name(data['intf-port-list'])

        if base and plat.mgmt_intf_base.startswith(base):
            port_list = parse_port_list(data['intf-port-list'],
                                        base,
                                        plat.mgmt_intf_base,
                                        plat.mgmt_intf_min,
                                        plat.mgmt_intf_max)
            for port in port_list:
                command.action_invoke('implement-show-mgmt-intf',
                                      ({'ifname': port},))
        elif base and plat.dp_intf_base.startswith(base):
            port_list = parse_port_list(data['intf-port-list'],
                                        base,
                                        plat.dp_intf_base,
                                        plat.dp_intf_min,
                                        plat.dp_intf_max)
            show_dp_intf_list(port_list, 'detail' in data)
        elif base and plat.lag_intf_base.startswith(base):
            port_list = parse_port_list(data['intf-port-list'],
                                        base,
                                        plat.lag_intf_base,
                                        plat.lag_intf_min,
                                        plat.lag_intf_max)
            show_dp_intf_list(port_list, 'detail' in data)
        else:
            ptypes = [plat.dp_intf_base, plat.lag_intf_base, plat.mgmt_intf_base]
            raise error.ActionError('Port type not in ' + str(ptypes))
    else:
        show_dp_intf_list([], False)

command.add_action('implement-show-intf', show_intf,
                   {'kwargs': { 'data'      : '$data', } } )


SHOW_INTERFACE_COMMAND_DESCRIPTION = {
    'name'         : 'show',
    'mode'         : 'login',
    'no-supported' : False,
    'doc'          : 'interface|show',
    'doc-example'  : 'interface|show-example',
    'action'       : 'implement-show-intf',
    'args'         : (
        {
            'token'           : 'interface',
            'short-help'      : 'Show interface parameters',
        },
        {
            'optional'        : True,
            'args'            : (
                {
                    'field'           : 'intf-port-list',
                    'type'            : 'interface-list',
                    'syntax-help'     : 'Interface port range list',
                    'doc'             : 'interface|show-intf-port-list',
                },
                {
                    'token'           : 'detail',
                    'short-help'      : 'detailed interface information',
                    'optional'        : True,
                    'doc'             : 'interface|show-intf-detail',
                    'data'            : { 'detail' : True },
                },
            ),
        },
    )
}


def shutdown_intf(no_command, port_list):
    # assumes port list has already been checked by caller
    ports = get_port_info()

    conn = OFConnection.OFConnection('127.0.0.1', 6634)

    for pname in port_list:
        port = ports[pname]
        req = of10.message.port_mod(xid=conn._gen_xid(),
                                    port_no=port[0].port_no,
                                    hw_addr=port[0].hw_addr,
                                    config=0 if no_command \
                                        else of10.OFPPC_PORT_DOWN,
                                    mask=of10.OFPPC_PORT_DOWN, 
                                    advertise=port[0].advertised)
        conn.sendmsg(req)

    # check for error messages
    msg = None
    try:
        msg = conn.recvmsg(timeout=1)
    except:
        if msg and msg.type == of10.OFPT_ERROR \
                and msg.err_type == OFPET_PORT_MOD_FAILED:
            raise error.ActionError('Error changing port state')

    conn.close()


def config_intf(no_command, data):
    base = get_base_name(data['intf-port-list'])

    if base and plat.mgmt_intf_base.startswith(base):
        port_list = parse_port_list(data['intf-port-list'], 
                                    base,
                                    plat.mgmt_intf_base,
                                    plat.mgmt_intf_min, 
                                    plat.mgmt_intf_max)
        for port in port_list:
            data['ifname'] = port
            command.action_invoke('implement-config-mgmt-intf', 
                                  (no_command, data))
    elif base and plat.dp_intf_base.startswith(base):
        port_list = parse_port_list(data['intf-port-list'], 
                                    base,
                                    plat.dp_intf_base,
                                    plat.dp_intf_min, 
                                    plat.dp_intf_max)
        if 'shutdown' in data:
            shutdown_intf(no_command, port_list)
        else:
            raise error.ActionError('Invalid action for dataplane interfaces')
    else:
        ptypes = [plat.dp_intf_base, plat.mgmt_intf_base]
        raise error.ActionError('Port type not in ' + str(ptypes))

command.add_action('implement-config-intf', config_intf,
                    {'kwargs': {
                                 'no_command' : '$is-no-command',
                                 'data'       : '$data',
                               } } )

CONFIG_IF_COMMAND_DESCRIPTION = {
    'name'         : 'interface',
    'mode'         : 'config',
    'short-help'   : 'Configure interface parameters',
    'action'       : 'implement-config-intf',
    'no-action'    : 'implement-config-intf',
    'doc'          : 'interface|interface',
    'doc-example'  : 'interface|interface-example',
    'args'         : (
        {
            'field'           : 'intf-port-list',
            'type'            : 'interface-list',
            'syntax-help'     : 'Interface port range list',
            'doc'             : 'interface|config-intf-port-list',
        },
        {
            'choices' : (
                (
                    {
                        'token'           : 'shutdown',
                        'data'            : { 'shutdown' : True },
                        'short-help'      : 'Shut down interface',
                        'doc'             : 'interface|shutdown',
                    },
                ),
                (
                    {
                        'token'           : 'ip-address',
                        'short-help'      : 'Configure IPv4 address',
                        'doc'             : 'mgmt|ip-address',
                    },
                    {
                        'choices' : (
                            (
                                {
                                    'token'           : 'dhcp',
                                    'data'            : { 'dhcp' : True },
                                    'short-help'      : 'Enable DHCP',
                                    'doc'             : 'mgmt|dhcp'
                                },
                                {
                                    'token'           : 'sticky',
                                    'data'            : { 'sticky' : True },
                                    'optional'        : True,
                                    'optional-for-no' : True,
                                    'short-help'      : 'Persist info regardless of lease duration',
                                },
                            ),
                            (
                                {
                                    'field'           : 'ip-address',
                                    'type'            : 'cidr-range',
                                    'syntax-help'     : 'IP address/prefix',
                                    'doc'             : 'mgmt|address',
                                },
                            ),
                        ),
                    },
                ),
            ),
        },
    ),
}


def clear_interface_statistics(data):
    try:
        shell.call('ofad-ctl clear-port-stats')
    except subprocess.CalledProcessError:
        raise error.ActionError('Error clearing interface statistics')

command.add_action('implement-clear-interface-statistics',
                   clear_interface_statistics,
                    {'kwargs': {'data'      : '$data',}})

CLEAR_INTERFACE_COMMAND_DESCRIPTION = {
    'name'         : 'clear',
    'mode'         : 'enable',
    'action'       : 'implement-clear-interface-statistics',
    'no-supported' : False,
    'args'         : (
        {
            'token'      : 'interface',
            'short-help' : 'Clear interface parameters',
            'doc'        : 'interface|clear',
        },
        {
            'token'      : 'statistics',
            'short-help' : 'Clear interface statistics',
            'doc'        : 'interface|clear-statistics',
        },
    )
}


def running_config_interface(context, runcfg, words):
    comp_runcfg = []

    # collect component-specific config
    ports = get_port_info()
    for port in ports.itervalues():
        if port[0].config & of10.OFPPC_PORT_DOWN:
            comp_runcfg.append('interface %s shutdown\n' % port[0].name)

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
            'values'     : 'dp-interface',
            'short-help' : 'Configuration for dataplane interfaces',
            'doc'        : 'running-config|show-dp-interface',
        },
    ),
)

run_config.register_running_config('dp-interface', 5500,  None,
                                   running_config_interface,
                                   interface_running_config_tuple)

