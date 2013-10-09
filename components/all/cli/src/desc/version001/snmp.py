# Copyright (c) 2013  BigSwitch Networks

import command
import run_config
import subprocess
from sl_util import Service
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

TEMP_SENSORS        = 'temp_sensors'
CHASSIS_FAN_SENSORS = 'chassis_fan_sensors'
POWER_FAN_SENSORS    = 'power_fan_sensors'
POWER_SENSORS        = 'power_sensors'

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

Platform = None

OID_Table = {
    'quanta-lb9': {
        TEMP_SENSORS : {
            'ctemp1' : '.1.3.6.1.4.1.2021.13.16.2.1.3.1',
            'ctemp2' : '.1.3.6.1.4.1.2021.13.16.2.1.3.5',
            'ctemp3' : '.1.3.6.1.4.1.2021.13.16.2.1.3.9',
            'ctemp4' : '.1.3.6.1.4.1.2021.13.16.2.1.3.13',
            'ctemp5' : '.1.3.6.1.4.1.2021.13.16.2.1.3.17',
            'pwr-temp1' : '.1.3.6.1.4.1.2021.13.16.2.1.3.41',
            'pwr-temp2' : '.1.3.6.1.4.1.2021.13.16.2.1.3.44',
            'pwr-temp3' : '.1.3.6.1.4.1.2021.13.16.2.1.3.46',
        },
        CHASSIS_FAN_SENSORS : {
            'cfan1' : '.1.3.6.1.4.1.2021.13.16.3.1.3.1',
            'cfan2' : '.1.3.6.1.4.1.2021.13.16.3.1.3.5',
            'cfan3' : '.1.3.6.1.4.1.2021.13.16.3.1.3.9',
            'cfan4' : '.1.3.6.1.4.1.2021.13.16.3.1.3.13',
        },
        POWER_FAN_SENSORS : {
            'pwr-fan' : '.1.3.6.1.4.1.2021.13.16.3.1.3.33',
        },
        POWER_SENSORS : {
            'power' : '.1.3.6.1.4.1.2021.13.16.5.1.3.8'
        }
    },

    'quanta-ly2': {
        TEMP_SENSORS : {
            'ctemp1' : '.1.3.6.1.4.1.2021.13.16.2.1.3.1',
            'ctemp2' : '.1.3.6.1.4.1.2021.13.16.2.1.3.2',
            'ctemp3' : '.1.3.6.1.4.1.2021.13.16.2.1.3.3',
            'ctemp4' : '.1.3.6.1.4.1.2021.13.16.2.1.3.4',
            'ctemp5' : '.1.3.6.1.4.1.2021.13.16.2.1.3.5',
            'pwr-temp6' : '.1.3.6.1.4.1.2021.13.16.2.1.3.6',
            'pwr-temp7' : '.1.3.6.1.4.1.2021.13.16.2.1.3.9',
            'pwr-temp8' : '.1.3.6.1.4.1.2021.13.16.2.1.3.14',
        },
        CHASSIS_FAN_SENSORS : {
            'cfan1' : '.1.3.6.1.4.1.2021.13.16.3.1.3.1',
            'cfan2' : '.1.3.6.1.4.1.2021.13.16.3.1.3.2',
            'cfan3' : '.1.3.6.1.4.1.2021.13.16.3.1.3.3',
            'cfan4' : '.1.3.6.1.4.1.2021.13.16.3.1.3.4',
        },
        POWER_FAN_SENSORS : {
            'pwr-fan' : '.1.3.6.1.4.1.2021.13.16.3.1.3.5',
        },
        POWER_SENSORS : {
            'power' : '.1.3.6.1.4.1.2021.13.16.5.1.3.1'
        }
    }
}

Mon_Ops = {
    TEMP_SENSORS        : '>',
    CHASSIS_FAN_SENSORS : '<',
    POWER_FAN_SENSORS   : '<',
    POWER_SENSORS       : '>'
}

def trap_set(no_cmd, trap, threshold):

    global Platform

    if Platform is None:
        f = open("/etc/sl_platform")
        Platform = f.readlines()[0].strip()
        f.close()

    oids = OID_Table.get(Platform)
    if oids == None:
        # TODO: Need grateful error message
        # raise error: The particular msg doesn't show
        print "Trap unsupported on %s" % (Platform)
        raise error.ActionError("Trap unsupported on %s", Platform)

    trapdict = oids.get(trap)
    if trapdict == None:
        # TODO: Need grateful error message
        print "Trap %s unsupported on %s" % (trap, Platform)
        raise error.ActionError("Trap %s unsupported on %s", trap, Platform)

    items = trapdict.items()
    for item in items:
        config_line(no_cmd,
                "monitor -I %s %s %s %d\n" % (item[0], item[1], Mon_Ops[trap], threshold))


def config_snmp(no_command, data):
    if 'host' in data:
        trap_dest(no_command,
                  data['host'],
                  data['notification'],
                  data['community'],
                  data['port']
                  )

    elif 'trap' in data:
        trap_set(no_command,
                  data['trap'],
                  data['threshold']
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
                (
                    {
                        'field'           : 'trap',
                        'tag'             : 'trap',
                        'short-help'      : 'Trap type',
                        'type'            : 'enum',
                        'values'          : (TEMP_SENSORS, CHASSIS_FAN_SENSORS,
                                             POWER_FAN_SENSORS, POWER_SENSORS),
                        'doc'             : 'snmp|+',
                    },
                    {
                        'field'           : 'threshold',
                        'tag'             : 'threshold',
                        'short-help'      : 'Threashold value',
                        'base-type'       : 'integer',
                        #'range'           : (0, 100),
                        #'doc'             : 'snmp|', #DO WE NEED IT
                    },
                ),
            ), # snmp choices: enable, host, location, trap
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
        if w[0] == 'monitor':
            trap_type = None
            if w[-4] == 'ctemp1':
                trap_type = TEMP_SENSORS
            elif w[-4] == 'cfan1':
                trap_type = CHASSIS_FAN_SENSORS
            elif w[-4] == 'pwr-fan':
                trap_type = POWER_FAN_SENSORS
            elif w[-4] == 'power':
                trap_type = POWER_SENSORS

            if trap_type != None:
                comp_runcfg.append('snmp-server trap trap-type %s threshold %s\n' %
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

