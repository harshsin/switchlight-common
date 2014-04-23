# Copyright (c) 2013-2014 BigSwitch Networks
# All rights reserved.

import command
import run_config
import error
import utif

import subprocess
from sl_util import shell
from sl_util import OFConnection
from sl_util import const
from sl_util.ofad import OFADConfig, PortManager

import loxi.of10 as of10

import re
import itertools

OFAgentConfig = OFADConfig()
PortManager.setPhysicalBase(OFAgentConfig.physical_base_name)
PortManager.setLAGBase(OFAgentConfig.lag_base_name)

# generate a regexp that only requires the first character of the name,
# with all other characters are optional;
# this is done by adding a '?' (match zero or one times) after all characters
# except the first.
def opt_re(name):
    s = [ name[0:2] ]  # no '?' after first character
    for c in name[2:]:
        if c in '-':   # escape special characters
            s.append('\\'+c)
        else:
            s.append(c)
    return '?'.join(s) + '?'


command.add_typedef({
        'name'      : 'interface-list',
        'help-name' : 'comma-separated list of port ranges',
        'base-type' : 'string',
        'pattern'   : r'^'+ opt_re(OFAgentConfig.physical_base_name) + '|' + 
        opt_re(OFAgentConfig.lag_base_name) +'(\d+(-\d+)?,)*\d+(-\d+)?$',
        })


def get_base_name(s):
    # given a string, return the base name of an interface; 
    # FIXME assumes it's all text...
    match = re.match(r'([A-Za-z-])+', s)
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
        format_str = "%2s%s %-14s %-5s %20s %20s %7s"
        print '* = Link up, D = Disabled'
        print format_str % ('#', ' ', 'Name', 'Speed', 
                            'Rx Packets', 'Tx Packets', '  ')

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

def parse_port_list(orig_port_list):
    portMgr = PortManager(OFAgentConfig.port_list)

    # extract the interface base name
    short_base = get_base_name(orig_port_list)
    all_bases = [const.MGMT_PORT_BASE] + PortManager.getAllPortBases()
    base = None
    for b in all_bases:
        if b.startswith(short_base):
            base = b
            break
    if base is None:
        raise error.ActionError('Port type not in %s' % all_bases)

    # resolve port list
    port_spec = orig_port_list[len(short_base):]
    port_nums = utif.resolve_port_list(port_spec)
    port_list = ['%s%d' % (base, n) for n in port_nums]

    # check that each port exists
    all_ports = const.MGMT_PORTS + portMgr.getExistingPorts()
    for port in port_list:
        if port not in all_ports:
            raise error.ActionError('%s is not an existing interface' % port)

    return (base, port_list)

def show_intf(data):
    if 'intf-port-list' in data:
        base, port_list = parse_port_list(data['intf-port-list'])

        if base == const.MGMT_PORT_BASE:
            for port in port_list:
                command.action_invoke('implement-show-mgmt-intf',
                                      ({'ifname': port},))
        else:
            show_dp_intf_list(port_list, 'detail' in data)
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
            'choices' : (
                (
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
                (
                    {
                        'field'           : 'intf-port-list',
                        'type'            : 'enum',
                        'values'          : const.MGMT_PORTS,
                        'syntax-help'     : 'Management interface',
                        'doc'             : 'interface|config-intf-port-list',
                    },
                    {
                        'token'           : 'detail',
                        'short-help'      : 'detailed interface information',
                        'optional'        : True,
                        'doc'             : 'interface|show-intf-detail',
                        'data'            : { 'detail' : True },
                    },
                ),
            ),
        },
    )
}


def shutdown_intf(no_command, port_list, is_init):
    portMgr = PortManager(OFAgentConfig.port_list)

    # assumes port list has already been checked by caller
    for port in port_list:
        if no_command:
            portMgr.enablePort(port)
        else:
            portMgr.disablePort(port)

    OFAgentConfig.port_list = portMgr.toJSON()
    OFAgentConfig.write(warn=True)
    OFAgentConfig.reload(deferred=is_init)

def config_intf(no_command, data, is_init):
    base, port_list = parse_port_list(data['intf-port-list'])

    if base == const.MGMT_PORT_BASE:
        for port in port_list:
            data['ifname'] = port
            command.action_invoke('implement-config-mgmt-intf', 
                                  (no_command, data))
    else:
        if 'shutdown' in data:
            shutdown_intf(no_command, port_list, is_init)
        else:
            raise error.ActionError('Invalid action for dataplane interfaces')

command.add_action('implement-config-intf', config_intf,
                    {'kwargs': {
                                 'no_command' : '$is-no-command',
                                 'data'       : '$data',
                                 'is_init'    : '$is-init',
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
            'token'           : 'shutdown',
            'data'            : { 'shutdown' : True },
            'short-help'      : 'Shut down interface',
            'doc'             : 'interface|shutdown',
        },
    ),
}


CONFIG_MGMT_IF_COMMAND_DESCRIPTION = {
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
            'type'            : 'enum',
            'values'          : const.MGMT_PORTS,
            'syntax-help'     : 'Management interface',
            'doc'             : 'interface|config-intf-port-list',
        },
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
    portMgr = PortManager(OFAgentConfig.port_list)

    # get all dataplane ports
    port_list = sorted(portMgr.getPhysicals(), key=lambda x: x.portName) + \
                sorted(portMgr.getLAGs(), key=lambda x: x.portName)

    # collect component-specific config
    # FIXME: generate "compressed" interface lists (e.g. eth1-10,13)
    for port in port_list:
        if port.disableOnAdd:
            comp_runcfg.append('interface %s shutdown\n' % port.portName)

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

# FIXME: running config of interface has to be after port-channel because the
#        port-channel command has to run first to create a lag before the
#        interface command (e.g. shutdown) can run against a lag.
#
#        the proper fix is to manage only phys with the interface command, and
#        only lags with the port-channel command.
run_config.register_running_config('dp-interface', 6500,  None,
                                   running_config_interface,
                                   interface_running_config_tuple)

