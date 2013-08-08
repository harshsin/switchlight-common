# Copyright (c) 2013  BigSwitch Networks

import command
import run_config
import subprocess
from PandOS import Service
import utif
import cfgfile
import error


# FIXME STUB
SNMP_CONFIG_FILE = '/etc/snmp/snmpd.conf'
#SNMP_CONFIG_FILE = './unittest/snmpd.conf'

# FIXME STUB
#SNMP_SYS_DESC = "Indigo v0.01"
# FIXME verify the following values are necessary for snmpd.conf
# agentAddress udp::161,udp6:[::1]:161
#BSN_ENTERPRISE_OID = '.1.3.6.1.4.1.37538'
#BSN_ENTERPRISE_OID_SWITCH = BSN_ENTERPRISE_OID + '.2'
# sysObjectID BSN_ENTERPRISE_OID_SWITCH
# sysDescr SNMP_SYS_DESC


class Snmp(Service):
    SVC_NAME = "snmpd"

def get_snmp_status():
    st = Snmp.status()
    if st == Service.HALTED:
        status = 'disabled'
    elif st == Service.RUNNING:
        status = 'enabled'
    else:
        status = "ERROR"
    return status

# datakeywords to parse CLI data dict parameter.
# snmpkeywords to parse snmpd.conf.
# datakeywords and snmpkeywords must be in 1-to-1 correspondance
#   e.g. value of data['location'] is value of 'sysLocation' in snmpd.conf
datakeywords = ['contact', 'location']
snmpkeywords = ['sysContact', 'sysLocation']

def parse_snmpd_conf(lines):
    """
    Return a dict whose keys are those in "datakeywords" above,
    and whose values are (associated value, index in "lines") tuples.
    """
    dt = {}
    for (idx, line) in enumerate(lines):
        tokens = line.split(None, 1)
        if len(tokens) == 2 and tokens[0] in snmpkeywords:
            # FIXME handle rocommunity/rwcommunity additional params?
            val = tokens[1].strip()
            dt[datakeywords[snmpkeywords.index(tokens[0])]] = (val, idx)
    return dt


def show_snmp_server(data):
    try:
        with cfgfile.FileLock(SNMP_CONFIG_FILE) as f:
            lines = cfgfile.get_line_list_from_file(f)
    except:
        raise error.ActionError('Cannot access SNMP configuration')

    cfg = parse_snmpd_conf(lines)

    print "status:  %s" % get_snmp_status()
    print "configuration:"
    for key in datakeywords:
        if key in cfg:
            print '  %-12s  %s' % (key, cfg[key][0])
    print '  communities:'
    for  li in lines:
        w = li[0:-1].split(' ')
        if w[0] == 'rocommunity' or w[0] == 'rwcommunity':
            print '    %s %s' % (w[1], w[0][0:2])
    print '  trap/inform destinations:'
    for li in lines:
        w = li[0:-1].split(' ')
        if w[0] == 'trap2sink':
            ww = w[1].split(':')
            print '    host %s traps %s udp-port %s' % (ww[0], w[2], ww[1])
            continue
        if w[0] == 'informsink':
            ww = w[1].split(':')
            print '    host %s informs %s udp-port %s' % (ww[0], w[2], ww[1])
            continue


command.add_action('implement-show-snmp-server', show_snmp_server,
                    {'kwargs': {'data'      : '$data',}})

SHOW_SNMP_SERVER_COMMAND_DESCRIPTION = {
    'name'         : 'show',
    'mode'         : 'login',
    'action'       : 'implement-show-snmp-server',
    'no-supported' : False,
    'args'         : (
        {
            'token'       : 'snmp-server',
            'short-help'  : 'Show the current SNMP server configuration',
            'doc'         : 'snmp|show',
        },
    )
}


def enable_snmp(no_command, data):
    if no_command:
        try:
            Snmp.disable()
        except:
            raise error.ActionError('Cannot disable SNMP service')
    else:
        try:
            Snmp.enable()
        except:
            raise error.ActionError('Cannot enable SNMP service')


command.add_action('implement-enable-snmp', enable_snmp,
                    {'kwargs': {
                                 'no_command' : '$is-no-command',
                                 'data'       : '$data',
                               } } )


# Add or remove the given line from the config file    
def config_line(no_cmd, li):
    try:
        with cfgfile.FileLock(SNMP_CONFIG_FILE) as f:
            lines = cfgfile.get_line_list_from_file(f)
        
            if no_cmd:
                # Remove matching line, if present

                if li in lines:
                    lines.remove(li)
                    cfgfile.put_line_list_to_file(f, lines)
            else:
                # Add matching line, if not present

                if li not in lines:
                    lines.append(li)
                    cfgfile.put_line_list_to_file(f, lines)
    except:
        raise error.ActionError('Cannot access SNMP configuration')


# Add or remove a trap/inform destination
def trap_dest(no_cmd, ip_addr, type, community, port):
    # Compose config file line for trap/inform dest,
    # and add it or remove it from the config file
    config_line(no_cmd,
                "%s %s:%s %s\n" % ('trap2sink' if type == 'traps' else 'informsink',
                                   ip_addr,
                                   str(port),
                                   community
                                   )
                )


# Add or remove a community
def community(no_cmd, community, access):
    # Compose config file line for community,
    # and add it or remove it from the config file
    config_line(no_cmd,
                "%scommunity %s\n" % (access, community)
                )
        

def config_snmp(no_command, data):
    if 'host' in data:
        trap_dest(no_command,
                  data['host'],
                  data['notification'],
                  data['community'],
                  data['port']
                  )

    elif 'access' in data:
        community(no_command, data['community'], data['access'])

    else:
        try:
            with cfgfile.FileLock(SNMP_CONFIG_FILE) as f:
                lines = cfgfile.get_line_list_from_file(f)
                cfg = parse_snmpd_conf(lines)

                for key in datakeywords:
                    if key in data:
                        if no_command:
                            if (key in cfg and 
                                (data[key] == '' or data[key] == cfg[key][0])):
                                del lines[cfg[key][1]]
                        else:
                            val = data[key]
                            newline = '%s %s\n' % \
                                (snmpkeywords[datakeywords.index(key)], val)
                            if key in cfg:
                                lines[cfg[key][1]] = newline
                            else:
                                lines.append(newline)

                cfgfile.put_line_list_to_file(f, lines)
        except:
            raise error.ActionError('Cannot access SNMP configuration')

    if get_snmp_status() == 'enabled':
        Snmp.restart()


command.add_action('implement-config-snmp', config_snmp,
                    {'kwargs': {
                                 'no_command' : '$is-no-command',
                                 'data'       : '$data',
                               } } )

SNMP_SERVER_COMMAND_DESCRIPTION = {
    'name'         : 'snmp-server',
    'mode'         : 'config',
    'short-help'   : 'Configure the SNMP server',
    'action'       : 'implement-config-snmp',
    'no-action'    : 'implement-config-snmp',
    'doc'          : 'snmp|snmp-server',
    'doc-example'  : 'snmp|snmp-server-example',
    'args'         : (
        {
            'choices': (
                (
                    {
                        'token'      : 'enable',
                        'short-help' : 'Enable SNMP',
                        'doc'        : 'snmp|snmp-server-enable',
                        'action'     : 'implement-enable-snmp',
                        'no-action'  : 'implement-enable-snmp',
                    },
                ),
                (
                    {
                        'token'           : 'community',
                        'short-help'      : 'Set community string and access privs',
                        'doc'             : 'snmp|snmp-server-community',
                    },
                    {
                        'field'           : 'access',
                        'short-help'      : 'Type of access with this community string',
                        'type'            : 'enum',
                        'values'          : ('rw', 'ro'),
                        'doc'             : 'snmp|+',
                    },
                    {
                        'field'           : 'community',
                        'type'            : 'string',
                        'optional-for-no' : True,
                        'syntax-help'     : 'Value for the SNMP community string',
                    },
                ),
                (
                    {
                        'token'           : 'location',
                        'short-help'      : 'Text for mib object sysLocation',
                        'doc'             : 'snmp|snmp-server-location',
                        'data'            : { 'location' : '' },
                    },
                    {
                        'field'           : 'location',
                        'type'            : 'string',
                        'optional-for-no' : True,
                        'syntax-help'     : 'Value for the SNMP location string',
                    },
                ),
                (
                    {
                        'token'           : 'contact',
                        'short-help'      : 'Text for mib object sysContact',
                        'doc'             : 'snmp|snmp-server-contact',
                        'data'            : { 'contact' : '' },
                    },
                    {
                        'field'           : 'contact',
                        'type'            : 'string',
                        'optional-for-no' : True,
                        'syntax-help'     : 'Value for the SNMP contact string',
                    },
                ),
                (
                    {
                        'token'           : 'host',
                        'short-help'      : 'Host to receive traps or informs',
                        'doc'             : 'snmp|snmp-server-host',
                    },
                    {
                        'field'           : 'host',
                        'type'            : 'ip-address-or-domain-name',
                    },
                    {
                        'field'           : 'notification',
                        'short-help'      : 'Type of notification',
                        'type'            : 'enum',
                        'values'          : ('traps', 'informs'),
                        'doc'             : 'snmp|+',
                    },
                    {
                        'field'           : 'community',
                        'base-type'       : 'identifier',  # FIXME change type, with 'reserved' field
                        'syntax-help'     : 'SNMP community string for notification',
                        'reserved'        : [ 'udp-port' ],
                    },
                    {
                        'token'           : 'udp-port',
                        'syntax-help'     : 'UDP port for notification',
                        'doc'             : 'snmp|snmp-server-udp-port',
                    },
                    {
                        'field'           : 'port',
                        'type'            : 'port',
                        'syntax-help'     : 'UDP port for notification',
                    },
                ),
            ),
        },
    ),
}


def running_config_snmp(context, runcfg, words):
    comp_runcfg = []

    try:
        with cfgfile.FileLock(SNMP_CONFIG_FILE) as f:
            lines = cfgfile.get_line_list_from_file(f)
    except:
        raise error.ActionError('Cannot access SNMP configuration')
    cfg = parse_snmpd_conf(lines)

    # collect component-specific config
    if get_snmp_status() == 'enabled':
        comp_runcfg.append('snmp-server enable\n')
    if 'location' in cfg:
        comp_runcfg.append('snmp-server location %s\n' %
                           utif.quote_string(cfg['location'][0]))
    if 'contact' in cfg:
        comp_runcfg.append('snmp-server contact %s\n' %
                           utif.quote_string(cfg['contact'][0]))

    for li in lines:
        w = li[0:-1].split(' ')
        if w[0] == 'rocommunity' or w[0] == 'rwcommunity':
            comp_runcfg.append('snmp-server community %s %s\n' %
                               (w[0][0:2], w[1])
                               )
            continue
        if w[0] == 'trap2sink':
            ww = w[1].split(':')
            comp_runcfg.append('snmp-server host %s traps %s udp-port %s\n' %
                               (ww[0], w[2], ww[1])
                               )
            continue
        if w[0] == 'informsink':
            ww = w[1].split(':')
            comp_runcfg.append('snmp-server host %s informs %s udp-port %s\n' %
                               (ww[0], w[2], ww[1])
                               )
            continue

    # attach component-specific config
    if len(comp_runcfg) > 0:
        runcfg.append('!\n')
        runcfg += comp_runcfg

snmp_running_config_tuple = (
    (
        {
            'optional'   : False,
            'field'      : 'running-config',
            'type'       : 'enum',
            'values'     : 'snmp-server',
            'short-help' : 'Configuration for SNMP',
            'doc'        : 'running-config|show-snmp',
        },
    ),
)

run_config.register_running_config('snmp-server', 3000,  None,
                                   running_config_snmp,
                                   snmp_running_config_tuple)

