# Copyright (c) 2013  BigSwitch Networks

import os
import json

import subprocess
from sl_util import shell, conf_state

import cfgfile
from contextlib import contextmanager
import copy

import utif
import error
import command
import run_config

from sl_util.types import DPID
from sl_util.ofad import OFADConfig, Controller

import loxi.of13 as ofp13

conf_state.register_save("ofad", OFADConfig.save_default_settings)
conf_state.register_revert("ofad", OFADConfig.revert_default_settings)

OFAgentConfig = OFADConfig()

def print_stats(outstring):
    # filter out "local listening" statistics from the given output string
    dump = True
    for line in outstring.splitlines():
        if line.startswith('Stats'):
            dump = False if 'local listening connection' in line else True
        if dump:
            print line

def show_controller(data):
    printfunc = None
    if 'history' in data:
        name = 'controller connection history'
        cmd = 'ofad-ctl cxn show-log'
    elif 'statistics' in data:
        name = 'controller statistics'
        cmd = 'ofad-ctl stats'
        if 'detail' in data:
            cmd += ' detail'
        printfunc = print_stats
    else:
        name = 'controller status'
        cmd = 'ofad-ctl controller show CONNECT'

    try:
        (out, err, ret) = shell.call(cmd)
        if printfunc:
            printfunc(out)
        else:
            print out,
    except subprocess.CalledProcessError:
        raise error.ActionError('Cannot retrieve ' + name)

command.add_action('implement-show-controller', show_controller,
                    {'kwargs': {'data'      : '$data',}})

SHOW_CONTROLLER_COMMAND_DESCRIPTION = {
    'name'         : 'show',
    'mode'         : 'login',
    'action'       : 'implement-show-controller',
    'no-supported' : False,
    'args'         : (
        {
            'token'      : 'controller',
            'short-help' : 'Show controller connection status',
            'doc'        : 'ofad|show-controller',
        },
        {
            'optional'   : True,
            'choices' : (
                (
                    {
                        'token'      : 'statistics',
                        'data'       : { 'statistics' : True },
                        'short-help' : 'Show controller connection statistics',
                        'doc'        : 'ofad|show-controller-statistics',
                    },
                    {
                        'token'      : 'detail',
                        'data'       : { 'detail' : True },
                        'optional'   : True,
                        'short-help' : 'Show detailed controller connection statistics',
                        'doc'        : 'ofad|show-controller-statistics-detail',
                    },
                ),
                (
                    { 
                        'token'      : 'history',
                        'data'       : { 'history' : True },
                        'short-help' : 'Show controller connection history',
                        'doc'        : 'ofad|show-controller-history',
                    },
                ),
            ),
        },
    ),
}


def show_datapath(data):
    print 'datapath:'
    print '  %-12s  %s' % ('id', OFAgentConfig.dpid)
    print '  %-12s  %s' % ('description', OFAgentConfig.of_dp_desc)

command.add_action('implement-show-datapath', show_datapath,
                    {'kwargs': {'data'      : '$data',}})

SHOW_DATAPATH_COMMAND_DESCRIPTION = {
    'name'         : 'show',
    'mode'         : 'login',
    'action'       : 'implement-show-datapath',
    'no-supported' : False,
    'args'         : (
        {
            'token'       : 'datapath',
            'short-help'  : 'Show OpenFlow datapath parameters',
            'doc'         : 'ofad|show-datapath',
        },
    )
}

def config_controller(no_command, data, is_init):
    clist = OFAgentConfig.controllers

    if 'port' in data:
        port = data['port']
    else:
        port = ofp13.OFP_TCP_PORT

    con = Controller().setAddress(data['ip']).setPort(port).setProtocol('tcp').setStatic(True)
    if no_command:
        con.setStatic(False)

    matched = False
    for c in clist:
        if c.merge(con):
            matched = True
        if con == c:
            matched = True

    if not matched:
        clist.append(con)

    OFAgentConfig.controllers = clist
    OFAgentConfig.write(warn=True)
    OFAgentConfig.reload(deferred=is_init)

command.add_action('implement-config-controller', config_controller,
                    {'kwargs': {
                                 'no_command' : '$is-no-command',
                                 'data'       : '$data',
                                 'is_init'    : '$is-init',
                               } } )

SET_CONTROLLER_COMMAND_DESCRIPTION = {
    'name'         : 'controller',
    'mode'         : 'config',
    'short-help'   : 'Configure a controller',
    'action'       : 'implement-config-controller',
    'no-action'    : 'implement-config-controller',
    'doc'          : 'ofad|controller',
    'doc-example'  : 'ofad|controller-example',
    'args'         : (
        {
            'field'           : 'ip',
            'type'            : 'ip-address',
            'optional'        : False,
            'completion-text' : 'controller ip address',
            'doc'             : 'ofad|controller-ip'
        },
        {
            'optional'        : True,
            'optional-for-no' : True,
            'args' : (
                {
                    'token'        : 'port',
                },
                {
                    'field'        : 'port',
                    'type'         : 'port',
                    'completion-text' : 'controller port',
                    'doc'             : 'ofad|controller-port'
                },
            ),
        },
    ),
}


def config_datapath(no_command, data, is_init):
    changed = False
    cfg = OFAgentConfig
    cfg.update()

    with OFAgentConfig.warnOnChanged():
        if 'id' in data:
            changed = True
            if no_command:
                cfg.dpid = DPID(cfg.of_mac_addr_base)
            else:
                cfg.dpid = DPID(data['id'])

        if 'desc' in data:
            changed = True
            if no_command:
                cfg.of_dp_desc = ''
            else:
                cfg.of_dp_desc = data['desc']

        if changed:
            OFAgentConfig.write()
            OFAgentConfig.reload(deferred=is_init)

command.add_action('implement-config-datapath', config_datapath,
                    {'kwargs': {
                                 'no_command' : '$is-no-command',
                                 'data'       : '$data',
                                 'is_init'    : '$is-init',
                               } } )

CONFIG_DATAPATH_COMMAND_DESCRIPTION = {
    'name'         : 'datapath',
    'mode'         : 'config',
    'short-help'   : 'Configure OpenFlow datapath parameters',
    'action'       : 'implement-config-datapath',
    'no-action'    : 'implement-config-datapath',
    'doc'          : 'ofad|datapath',
    'doc-example'  : 'ofad|datapath-example',
    'args'         : (
        {
            'choices' : (
                (
                    {
                        'token'           : 'id',
                        'short-help'      : 'Set OpenFlow datapath id',
                        'doc'             : 'ofad|datapath-id',
                        'data'            : { 'id': None },
                    },
                    {
                        'field'           : 'id',
                        'type'            : 'dpid',
                        'optional-for-no' : True,
                        'syntax-help'     : 'Up to 16 hexadecimal digits',
                    },
                ),
                (
                    {
                        'token'           : 'description',
                        'short-help'      : 'Set OpenFlow datapath description',
                        'doc'             : 'ofad|datapath-description',
                        'data'            : { 'desc': None },
                    },
                    {
                        'field'           : 'desc',
                        'type'            : 'dp-desc',
                        'optional-for-no' : True,
                        'syntax-help'     : 'Up to 255 characters',
                    },
                ),
            ),
        },
    ),
}


def config_logging(no_command, data, is_init):
    log = OFAgentConfig.logging

    if no_command:
        if data['module'] in log:
            if 'loglevel' in data and log[data['module']] != data['loglevel']:
                raise error.ActionError('Does not match configured value')
            del log[data['module']]
    else:
        log[data['module']] = data['loglevel']

    OFAgentConfig.logging = log
    OFAgentConfig.write(warn=True)
    OFAgentConfig.reload(deferred=is_init)

command.add_action('implement-config-openflow-logging', config_logging,
                    {'kwargs': {
                                 'no_command' : '$is-no-command',
                                 'data'       : '$data',
                                 'is_init'    : '$is-init',
                               } } )

OPENFLOW_LOGGING_COMMAND_DESCRIPTION = {
    'name'         : 'logging',
    'mode'         : 'config',
    # do not add short-help; all-help in rlog.py should take precedence
    'action'       : 'implement-config-openflow-logging',
    'no-action'    : 'implement-config-openflow-logging',
    'args'         : (
        {
            'field'           : 'module',
            'short-help'      : 'Configure logging for this component',
            'type'            : 'enum',
            'values'          : ('connection', 'flowtable', 'dataplane'),
            'doc'             : 'ofad|+',
        },
        {
            'field'           : 'loglevel',
            'short-help'      : 'Logging level for this module',
            'type'            : 'enum',
            'values'          : ('emergencies', 'alerts', 'critical', 
                                 'errors', 'warnings', 'notifications',
                                 'informational', 'debugging'),
            'optional-for-no' : True,
            'doc'             : 'ofad|+',
        },
    ),
}


TABLE_MISS_ACTION_DEFAULT = 'drop'

def config_table_miss_action(no_command, data, is_init):
    if no_command:
        if 'table-miss-action' in data and \
                OFAgentConfig.table_miss_action != data['table-miss-action']:
            raise error.ActionError('Does not match configured value')
        OFAgentConfig.table_miss_action = TABLE_MISS_ACTION_DEFAULT
    else:
        OFAgentConfig.table_miss_action = data['table-miss-action']

    OFAgentConfig.write(warn=True)
    OFAgentConfig.reload(deferred=is_init)

command.add_action('implement-config-table-miss-action', 
                   config_table_miss_action,
                    {'kwargs': {
                                 'no_command' : '$is-no-command',
                                 'data'       : '$data',
                                 'is_init'    : '$is-init',
                               } } )

OPENFLOW_TABLE_MISS_ACTION_COMMAND_DESCRIPTION = {
    'name'         : 'table-miss-action',
    'mode'         : 'config',
    'short-help'   : 'Configure table miss action',
    'action'       : 'implement-config-table-miss-action',
    'no-action'    : 'implement-config-table-miss-action',
    'doc'          : 'ofad|table-miss-action',
    'doc-example'  : 'ofad|table-miss-action-example',
    'args'         : (
        {
            'field'           : 'table-miss-action',
            'short-help'      : 'Action to take if no matching flows',
            'type'            : 'enum',
            'values'          : ('packet-in', 'drop', 'normal'),
            'optional-for-no' : True,
            'doc'             : 'ofad|+',
        },
    ),
}


def running_config_openflow(context, runcfg, words):
    comp_runcfg = []

    # collect component-specific config    
    for con in OFAgentConfig.controllers:
        cstr = 'controller ' + con.addr
        if con.port != ofp13.OFP_TCP_PORT:
            cstr += ' port ' + str(con.port)
        comp_runcfg.append(cstr + '\n')

    comp_runcfg.append('datapath id %s\n' % (OFAgentConfig.dpid))
    if OFAgentConfig.of_dp_desc:
        comp_runcfg.append('datapath description %s\n' % 
                           utif.quote_string(OFAgentConfig.of_dp_desc))

    for (module, loglevel) in OFAgentConfig.logging.iteritems():
        comp_runcfg.append('logging %s %s\n' % (module, loglevel))

    if OFAgentConfig.table_miss_action != TABLE_MISS_ACTION_DEFAULT:
        comp_runcfg.append('table-miss-action %s\n' % 
                           OFAgentConfig.table_miss_action)

    # attach component-specific config
    if len(comp_runcfg) > 0:
        runcfg.append('!\n')
        runcfg += comp_runcfg

openflow_running_config_tuple = (
    (
        {
            'optional'   : False,
            'field'      : 'running-config',
            'type'       : 'enum',
            'values'     : 'openflow',
            'short-help' : 'Configuration for openflow',
            'doc'        : 'running-config|show-openflow',
        },
    ),
)

run_config.register_running_config('openflow', 5000,  None,
                                   running_config_openflow,
                                   openflow_running_config_tuple)

