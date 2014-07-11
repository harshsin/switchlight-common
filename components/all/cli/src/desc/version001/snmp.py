# Copyright (c) 2013-2014 BigSwitch Networks

import command
import run_config
import subprocess
from sl_util import Service, utils, const
import utif
import cfgfile
import error
from switchlight.platform.base import *
from switchlight.platform.current import SwitchLightPlatform

Platform=SwitchLightPlatform()

LINK_UP_DOWN_CLI              = 'linkUpDown'
#Used to be: LINK_UP_DOWN_NOTIFICATION_CMD = 'linkUpDownNotifications'
#Now using other cmds to allow polling interval setting. See 'man snmpd.conf'
trapLinkUp    = '.1.3.6.1.6.3.1.1.5.4'
trapLinkDown  = '.1.3.6.1.6.3.1.1.5.3'
ifIndex       = '.1.3.6.1.2.1.2.2.1.1'
ifAdminStatus = '.1.3.6.1.2.1.2.2.1.7'
ifOperStatus  = '.1.3.6.1.2.1.2.2.1.8'
LINK_UP_NOTIFICATION   = 'notificationEvent linkUpTrap %s %s %s %s\n' % \
    (trapLinkUp, ifIndex, ifAdminStatus, ifOperStatus)
LINK_DOWN_NOTIFICATION = 'notificationEvent linkDownTrap %s %s %s %s\n' % \
    (trapLinkDown, ifIndex, ifAdminStatus, ifOperStatus)
LINK_UP_MONITOR   = 'monitor -r %%d -e linkUpTrap "Generate linkUp" %s != 2\n' % ifOperStatus
LINK_UP_MONITOR_TOKEN_NUM = LINK_UP_MONITOR.split(' ').__len__()
LINK_DOWN_MONITOR = 'monitor -r %%d -e linkDownTrap "Generate linkDown" %s == 2\n' % ifOperStatus

class Snmp(Service):
    SVC_NAME = "snmpd"
    CFG_PATH = const.SNMP_CFG_PATH

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
        with cfgfile.FileLock(Snmp.CFG_PATH) as f:
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
        with cfgfile.FileLock(Snmp.CFG_PATH) as f:
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


Mon_Ops = {
    oids.TEMP_SENSORS        : '>',
    oids.CHASSIS_FAN_SENSORS : '<',
    oids.POWER_FAN_SENSORS   : '<',
    oids.POWER_SENSORS       : '>',
    oids.CPU_LOAD            : '>',
    oids.MEM_TOTAL_FREE      : '<',
    oids.FLOW_TABLE_L2_UTILIZATION      : '>',
    oids.FLOW_TABLE_TCAM_FM_UTILIZATION : '>',
    oids.LINK_TABLE_UTILIZATION         : '>'
}

# Key must be matched to Platform.oid_table's
Show_Trap_Type_Conv = {
    'ctemp1' : oids.TEMP_SENSORS,
    'cfan1'  : oids.CHASSIS_FAN_SENSORS,
    'pwr-fan': oids.POWER_FAN_SENSORS,
    'power'  : oids.POWER_SENSORS,
    'cpuload': oids.CPU_LOAD,
    'memtotalfree' : oids.MEM_TOTAL_FREE,
    'ft_l2_utilization'      : oids.FLOW_TABLE_L2_UTILIZATION,
    'ft_tcam_fm_utilization' : oids.FLOW_TABLE_TCAM_FM_UTILIZATION,
    'ft_link_utilization'    : oids.LINK_TABLE_UTILIZATION
}

def trap_set(no_cmd, trap, threshold):
    oids = Platform.oid_table()
    if oids is None:
        raise error.ActionError("Trap unsupported on %s", Platform.platform())

    trapdict = oids.get(trap)
    if trapdict is None:
        raise error.ActionError("Trap %s unsupported on %s", trap, Platform.platform())

    items = trapdict.items()
    for item in items:
        config_line(no_cmd,
                "monitor -I %s %s %s %d\n" % (item[0], item[1], Mon_Ops[trap], threshold))


def config_snmp(no_command, data, is_init):
    if 'host' in data:
        trap_dest(no_command,
                  data['host'],
                  data['notification'],
                  data['community'],
                  data['port']
                  )

    elif 'trap' in data:
        if data['trap'] is LINK_UP_DOWN_CLI:
            config_line(no_command, LINK_UP_NOTIFICATION)
            config_line(no_command, LINK_DOWN_NOTIFICATION)
            config_line(no_command, LINK_UP_MONITOR % data['interval'])
            config_line(no_command, LINK_DOWN_MONITOR % data['interval'])
        else:
            trap_set(no_command,
                     data['trap'],
                     data['threshold']
                     )

    elif 'access' in data:
        community(no_command, data['community'], data['access'])

    else:
        try:
            with cfgfile.FileLock(Snmp.CFG_PATH) as f:
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
        Snmp.restart(deferred=is_init)


command.add_action('implement-config-snmp', config_snmp,
                    {'kwargs': {
                                 'no_command' : '$is-no-command',
                                 'data'       : '$data',
                                 'is_init'    : '$is-init',
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
                (
                    {
                        'field'           : 'trap',
                        'tag'             : 'trap',
                        'short-help'      : 'Trap type',
                        'type'            : 'enum',
                        'values'          : (oids.TEMP_SENSORS, oids.CHASSIS_FAN_SENSORS,
                                             oids.POWER_FAN_SENSORS, oids.POWER_SENSORS,
                                             oids.CPU_LOAD, oids.MEM_TOTAL_FREE,
                                             oids.FLOW_TABLE_L2_UTILIZATION,
                                             oids.FLOW_TABLE_TCAM_FM_UTILIZATION),
                        'doc'             : 'snmp|+',
                    },
                    {
                        'field'           : 'threshold',
                        'tag'             : 'threshold', #tag makes it 'as if' token
                        'short-help'      : 'Threshold value',
                        'base-type'       : 'integer',
                        'doc'             : 'snmp|snmp-threshold',
                    },
                ),
                (
                    {
                        'token'           : 'trap',
                        'short-help'      : 'Enable trap',
                    },
                    {
                        'token'           : LINK_UP_DOWN_CLI,
                        'short-help'      : 'Link up/down notification',
                        'data'            : { 'trap' : LINK_UP_DOWN_CLI }, #set data['trap']
                        'doc'             : 'snmp|snmp-linkUpDown',
                    },
                    {
                        'field'           : 'interval',
                        'tag'             : 'interval', #tag makes it 'as if' token
                        'short-help'      : 'Polling interval in seconds',
                        'base-type'       : 'integer',
                    },
                ),
            ), # snmp choices: enable, host, location, trap
        },
    ),
}


def running_config_snmp(context, runcfg, words):
    comp_runcfg = []

    try:
        with cfgfile.FileLock(Snmp.CFG_PATH) as f:
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
        if w[0] == 'monitor' and w.__len__() == LINK_UP_MONITOR_TOKEN_NUM and w[4] == 'linkUpTrap':
            comp_runcfg.append('snmp-server trap %s interval %s\n' %
                               (LINK_UP_DOWN_CLI, w[2])
                               )
            continue
        if w[0] == 'monitor':
            trap_type = Show_Trap_Type_Conv.get(w[-4])
            if trap_type is not None:
                comp_runcfg.append('snmp-server trap %s threshold %s\n' %
                                   (trap_type, w[-1])
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

