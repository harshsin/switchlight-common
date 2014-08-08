# Copyright (c) 2013-2014 BigSwitch Networks
# All rights reserved.

import command
import run_config
import error
import utif

import subprocess
from sl_util import shell, OFConnection, const, conf_state
from sl_util.ofad import OFADConfig, PortManager

import loxi.of13 as of13

import re
import itertools
import json
import os

OFAgentConfig = OFADConfig()
PortManager.setPhysicalBase(OFAgentConfig.physical_base_name)
PortManager.setLAGBase(OFAgentConfig.lag_base_name)

# generate a regexp that only requires the first character of the name,
# with all other characters are optional;
# this is done by adding a '?' (match zero or one times) after all characters
# except the first.
def opt_re(name):
    # no '?' after first character; escape special characters
    s = [ name[0:2] ] + [ '\\' + c if c in '-' else c for c in name[2:] ]
    return '?'.join(s) + '?'

all_bases = [ OFAgentConfig.physical_base_name, 
              OFAgentConfig.lag_base_name,
              const.MGMT_PORT_BASE ]
all_base_re = '|'.join([opt_re(b) for b in all_bases])

command.add_typedef({
        'name'      : 'interface-list',
        'help-name' : 'comma-separated list of port ranges',
        'base-type' : 'string',
        'pattern'   : r'^('+ all_base_re +')(\d+(-\d+)?,)*\d+(-\d+)?$',
        })


def get_base_name(s):
    # given a string, return the base name of an interface;
    match = re.match(r'(' + all_base_re + ')', s)
    return match.group() if match else ''

def get_full_base_name(all_bases, base):
    # given a string 'base' and the possible base names in 'all_bases',
    # return the matching full base name and '' if no such match exists
    for candidate in all_bases:
        if candidate.startswith(base):
            return candidate
    return ''

def get_port_name(s):
    # pyloxi returns the entire field,
    # but we only want the part of the string before the null
    return s.split('\x00')[0]

def get_port_info():
    # returns a dict of [port_desc, port_stats], keyed by port name

    conn = OFConnection.OFConnection('127.0.0.1', 6634)

    # store port_desc from port_desc_stats_request
    ports = {}
    num2name = {}
    for entrylist in conn.of13_multipart_request_generator(
            of13.message.port_desc_stats_request()):
        for entry in entrylist:
            entry.name = get_port_name(entry.name)
            ports[get_port_name(entry.name)] = [entry]
            num2name[entry.port_no] = get_port_name(entry.name)

    # merge in rx and tx packet counters
    for entrylist in conn.of13_multipart_request_generator(
            of13.message.bsn_port_counter_stats_request( \
            port_no=of13.OFPP_ALL)):
        for entry in entrylist:
            ports[num2name[entry.port_no]].append(entry.values)

    conn.close()

    return ports

UNSUPPORTED = 0xffffffffffffffff

def is_err(val):
    return val.value != UNSUPPORTED and val.value > 0

def display(val):
    return str(val.value) if val.value != UNSUPPORTED else '---'

def get_speed(bmap):
    if bmap & of13.OFPPF_40GB_FD:
        return '40G'
    elif bmap & of13.OFPPF_10GB_FD:
        return '10G'
    elif bmap & (of13.OFPPF_1GB_FD | of13.OFPPF_1GB_HD):
        return '1G'
    elif bmap & (of13.OFPPF_100MB_FD | of13.OFPPF_100MB_HD):
        return '100M'
    elif bmap & (of13.OFPPF_10MB_FD | of13.OFPPF_10MB_HD):
        return '10M'
    else:
        return ''

def show_one_dp_intf_detail(port):
    # prints detailed info for the given (port_desc, port_stats) tuple
    pd = port[0]
    ps = port[1]

    print '%s is %s' % \
        (pd.name,
         'admin down' if pd.config & of13.OFPPC_PORT_DOWN
         else 'down' if pd.state & of13.OFPPS_LINK_DOWN
         else 'up')
    print '  Hardware Address: %s' % of13.util.pretty_mac(pd.hw_addr)
    print '  Speed: %s' % get_speed(pd.curr)
    print '  Received %s bytes, %s packets' % \
        (display(ps[of13.OFP_BSN_PORT_COUNTER_RX_BYTES]),
         display(ps[of13.OFP_BSN_PORT_COUNTER_RX_PACKETS]))
    print '    %s broadcast, %s multicast' % \
        (display(ps[of13.OFP_BSN_PORT_COUNTER_RX_PACKETS_BROADCAST]),
         display(ps[of13.OFP_BSN_PORT_COUNTER_RX_PACKETS_MULTICAST]))
    print '    %s runt, %s giant' % \
        (display(ps[of13.OFP_BSN_PORT_COUNTER_RX_RUNTS]),
         display(ps[of13.OFP_BSN_PORT_COUNTER_RX_GIANTS]))
    print '    %s error, %s CRC, %s alignment' % \
        (display(ps[of13.OFP_BSN_PORT_COUNTER_RX_ERRORS]),
         display(ps[of13.OFP_BSN_PORT_COUNTER_RX_CRC_ERRORS]),
         display(ps[of13.OFP_BSN_PORT_COUNTER_RX_ALIGNMENT_ERRORS]))
    print '    %s symbol, %s discard, %s pause' % \
        (display(ps[of13.OFP_BSN_PORT_COUNTER_RX_SYMBOL_ERRORS]),
         display(ps[of13.OFP_BSN_PORT_COUNTER_RX_DROPPED]),
         display(ps[of13.OFP_BSN_PORT_COUNTER_RX_PAUSE_INPUT]))
    print '  Sent %s bytes, %s packets' % \
        (display(ps[of13.OFP_BSN_PORT_COUNTER_TX_BYTES]),
         display(ps[of13.OFP_BSN_PORT_COUNTER_TX_PACKETS]))
    print '    %s broadcast, %s multicast' % \
        (display(ps[of13.OFP_BSN_PORT_COUNTER_TX_PACKETS_BROADCAST]),
         display(ps[of13.OFP_BSN_PORT_COUNTER_TX_PACKETS_MULTICAST]))
    print '    %s error, %s collision' % \
        (display(ps[of13.OFP_BSN_PORT_COUNTER_TX_ERRORS]),
         display(ps[of13.OFP_BSN_PORT_COUNTER_TX_COLLISIONS]))
    print '    %s late collision, %s deferred' % \
        (display(ps[of13.OFP_BSN_PORT_COUNTER_TX_LATE_COLLISIONS]),
         display(ps[of13.OFP_BSN_PORT_COUNTER_TX_DEFERRED]))
    print '    %s discard, %s pause' % \
        (display(ps[of13.OFP_BSN_PORT_COUNTER_TX_DROPPED]),
         display(ps[of13.OFP_BSN_PORT_COUNTER_TX_PAUSE_OUTPUT]))

def show_one_dp_intf_summary(format_str, port):
    # prints summary info for the given (port_desc, port_stats) tuple
    pd = port[0]
    ps = port[1]

    if pd.config & of13.OFPPC_PORT_DOWN:
        state = 'D'
    elif (pd.state & of13.OFPPS_LINK_DOWN) == 0:
        state = '*'
    else:
        state = ' '
    if is_err(ps[of13.OFP_BSN_PORT_COUNTER_RX_ERRORS]) or \
      is_err(ps[of13.OFP_BSN_PORT_COUNTER_TX_ERRORS]) or \
      is_err(ps[of13.OFP_BSN_PORT_COUNTER_RX_CRC_ERRORS]) or \
      is_err(ps[of13.OFP_BSN_PORT_COUNTER_RX_ALIGNMENT_ERRORS]) or \
      is_err(ps[of13.OFP_BSN_PORT_COUNTER_RX_SYMBOL_ERRORS]):
        err = '+Errors'
    else:
        err = '      '
    print format_str % \
        (str(pd.port_no), state, get_port_name(pd.name), get_speed(pd.curr),
         display(ps[of13.OFP_BSN_PORT_COUNTER_RX_PACKETS]),
         display(ps[of13.OFP_BSN_PORT_COUNTER_TX_PACKETS]), err)

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
    if '1g-sfp' in data:
        try:
            c = json.load(open(const.BRCM_JSON))
            if 'port' in c:
                for (p,v) in c['port'].iteritems():
                    if '1gsfp' in v and v['1gsfp']:
                        print p
        except:
            print None

    elif 'intf-port-list' in data:
        base, port_list = parse_port_list(data['intf-port-list'])

        if base == const.MGMT_PORT_BASE:
            for port in port_list:
                command.action_invoke('implement-show-mgmt-intf',
                                      ({'ifname': port},))
        else:
            show_dp_intf_list(port_list, 'detail' in data)
    else:
        show_dp_intf_list([], False)



def complete_intf_list(prefix, completions):
    """
    Given the prefix typed by the user,
    updates the specified completions dictionary with possible completion
    key/value pairs, following the cli completion framework.
    """

    def intf_num(s, search_full_base):
        return s[len(search_full_base):]

    # interfaces: the space of all possible interfaces
    # search_full_base: full interface base name
    # search_intf_num: interface number prefix, can be empty or partial
    # prefix: user input to which completions are appended
    # completions: dictionary of completions, composed of prefix 
    #   with interface number
    # knock_outs: list of intf numbers that should not appear in completions
    def completions_startingwith(interfaces, search_full_base, search_intf_num,
                                 prefix, completions, knock_outs):
        result = [prefix + intf_num(x, search_full_base)
                  for x in interfaces 
                  if x.startswith(search_full_base+search_intf_num) and
                  intf_num(x, search_full_base) not in knock_outs]
        completions.update({x: "Known interface" for x in result})

    # return all completions starting with search_prefix and
    # greater than start_intf_num, 
    # up to the first relevant interface number in knock_outs;
    # assumes list of knock_outs is sorted
    def completions_greaterthan(interfaces, search_full_base,
                                search_prefix, start_intf_num,
                                prefix, completions, knock_outs):
        end_intf_list = \
            [k for k in knock_outs if int(k) > int(start_intf_num)]
        end_intf_num = end_intf_list[0] if end_intf_list else None

        result = [prefix + intf_num(x, search_full_base)
                  for x in interfaces 
                  if x.startswith(search_prefix) and 
                  int(intf_num(x, search_full_base)) > int(start_intf_num) and
                  (end_intf_num is None or 
                   int(intf_num(x, search_full_base)) < int(end_intf_num)) ]
        completions.update({x: "Known interface" for x in result})

    # returns (validate, search_intf, prefix, greaterthan)
    #   validate: port spec to be validated
    #   search_intf: next interface must start with this number
    #   prefix: prefix to be prepended to all choices
    #   greaterthan: if not None, interfaces must be greater than this
    def split_on_last_range(s):
        splits = s.rsplit(',',1)
        last_range = splits.pop()
        if '-' not in last_range:
            # last range is a single intf
            prefix = ','.join(splits + [''])
            return (''.join(splits), last_range, prefix, None)
        elif last_range[-1] == '-':
            # last range is 'intf-'
            start = last_range[:-1]
            prefix = ','.join(splits + [start + '-'])
            return (''.join(splits), '', prefix, start)
        else:
            # last range is 'intf-intf'
            (start, end) = last_range.split('-', 1)
            all_but_end = ','.join(splits + [start])
            prefix = all_but_end + '-'
            return (all_but_end, end, prefix, start)

    portMgr = PortManager(OFAgentConfig.port_list)
    ports = portMgr.getExistingPorts()
    ports.extend(const.MGMT_PORTS)
    all_interfaces = { x: x for x in ports }

    if prefix == '':
        completions_startingwith(all_interfaces, '', '', '', completions, [])
        return

    base = get_base_name(prefix)
    full_base = get_full_base_name(all_bases, base)
    if full_base == '':
        # bad base name, abort
        return

    port_spec = prefix[len(base):]
    if port_spec == '':
        completions_startingwith(all_interfaces, full_base, '', 
                                 prefix, completions, [])
        return

    (validate_list, search_intf, prepend, greater) = \
        split_on_last_range(port_spec)

    # validate port list
    try:
        # FIXME change resolve_port_list so [] is returned if port spec is ''
        port_list = utif.resolve_port_list(validate_list) if validate_list \
            else []
    except Exception as e:
        # bad port spec, abort
        return

    if greater is None:
        # all interfaces except knockouts
        completions_startingwith(all_interfaces, full_base, search_intf,
                                 base+prepend, completions,
                                 [str(intf) for intf in port_list])
        if prefix and prefix[-1] != ',':
            completions[prefix + '-'] = 'Range of interfaces'
    else:
        # all greater interfaces until first knockout
        completions_greaterthan(all_interfaces, full_base, 
                                full_base+search_intf, greater,
                                base+prepend, completions,
                                [str(intf) for intf in port_list])

    if prefix in completions:
        if len(completions):
            completions[prefix + ','] = 'List of interfaces'
        completions[prefix + ' <cr>'] = 'Current interface selection'


command.add_completion('complete-intf-list', complete_intf_list,
                       {'kwargs': {
                                   'prefix'       : '$text',
                                   'completions'  : '$completions',
                                   }})

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
                    {
                        'token'           : '1g-sfp',
                        'short-help'      : 'Show ports configured for 1G sfp.',
                        'data'            : { '1g-sfp' : True },
                    },
                (
                    {
                        'field'           : 'intf-port-list',
                        'type'            : 'interface-list',
                        'syntax-help'     : 'Interface port range list',
                        'doc'             : 'interface|show-intf-port-list',
                        'completion'      : 'complete-intf-list',
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


def sfp_1g_intf(no_command, port_list, is_init):
    data = {}
    if os.path.exists(const.BRCM_JSON):
        try:
            data = json.load(open(const.BRCM_JSON))
        except ValueError:
            pass
    if data is None:
        data = {}

    for port in port_list:
        # Fixme
        if port.startswith('ethernet'):
            port = port.replace('ethernet','')
            if no_command:
                try:
                    data['port'][port].remove('1gsfp')
                except:
                    pass
                shell.call('ofad-ctl autoneg %s 0' % port)
            else:
                if not 'port' in data:
                    data['port'] = {}
                if not port in data['port']:
                    data['port'][port] = {}
                data['port'][port]['1gsfp'] = True
                shell.call('ofad-ctl autoneg %s 1' % port)

    open(const.BRCM_JSON,'w').write(json.dumps(data))



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
        elif '1g-sfp' in data:
            sfp_1g_intf(no_command, port_list, is_init)
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
            'choices' : (
                {
                    'token'           : 'shutdown',
                    'data'            : { 'shutdown' : True },
                    'short-help'      : 'Shut down interface',
                    'doc'             : 'interface|shutdown',
                },
                {
                    'token'           : '1g-sfp',
                    'data'            : { '1g-sfp' : True },
                    'short-help'      : 'Assume 1G SFP',
                    'doc'             : 'interface|1g-sfp',
                },
            ),
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


def revert_default_interface():
    if os.path.exists(const.BRCM_JSON):
        os.unlink(const.BRCM_JSON)

conf_state.register_revert("intf", revert_default_interface)


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

