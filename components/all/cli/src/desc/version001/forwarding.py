# Copyright (c) 2013  BigSwitch Networks

import command
import run_config
import error

from sl_util.ofad import OFADConfig, ForwardingConfig

OFAgentConfig = OFADConfig()

def config_forwarding(no_command, data, is_init):
    fwdCfg = ForwardingConfig(OFAgentConfig.forwarding)

    if data['type'] == 'pimu':
        if no_command: # when run with a 'no' its an enable op
            fwdCfg.enablePIMU()
        else:
            fwdCfg.disablePIMU()

    elif data['type'] == 'l2cache':
        if no_command: # when run with a 'no' its a disable op
            fwdCfg.disableL2CACHE()
        else:
            fwdCfg.enableL2CACHE()
    elif data['type'] == 'pause':
        if no_command: # When run with a 'no' its a disable op
            fwdCfg.disablePause()
        else:
            fwdCfg.enablePause()
    else:
        raise error.ActionError('Unspported forwarding config type: %s' % data['type'])

    OFAgentConfig.forwarding = fwdCfg.toJSON()
    OFAgentConfig.write(warn=True)
    OFAgentConfig.reload(deferred=is_init)

command.add_action('implement-config-forwarding', config_forwarding,
                   {'kwargs': {
                       'no_command' : '$is-no-command',
                       'data'       : '$data',
                       'is_init'    : '$is-init',
                    }})

CONFIG_FORWARDING_COMMAND_DESCRIPTION = {
    'name'          : 'forwarding',
    'mode'          : 'config',
    'short-help'    : 'Configure Forwarding Settings',
    'action'        : 'implement-config-forwarding',
    'no-action'     : 'implement-config-forwarding',
    'doc'           : 'forwarding|forwarding',
    'args'          : (
        {
            'choices'   : (
                (
                    {
                        'token'         : 'pimu',
                        'short-help'    : 'Configure Packet-In Management Unit',
                    },
                    {
                        'token'         : 'disable',
                        'data'          : {'type' : 'pimu'},
                        'doc'           : 'forwarding|pimu-disable',
                    },
                ),
                (
                    {
                        'token'         : 'l2cache',
                        'short-help'    : 'Configurare L2 Cache',
                    },
                    {
                        'token'         : 'enable',
                        'data'          : {'type' : 'l2cache'},
                        'doc'           : 'forwarding|l2cache-enable',
                    },
                ),
                (
                    {
                        'token'         : 'pause',
                        'short-help'    : 'Configure port pause settings',
                    },
                    {
                        'token'         : 'enable',
                        'data'          : {'type' : 'pause'},
                        'doc'           : 'forwarding|pause-enable',
                    },
                ),

            ),
        },
    ),
}

SHOW_FORWARDING_COMMAND_DESCRIPTION = {
    'name'          : 'show',
    'mode'          : 'login',
    'short-help'    : 'Show Forwarding Configuration Status',
    'no-supported'  : False,
    'doc'           : 'forwarding|show',
    'args'          : (
        {
            'token'     : 'forwarding',
        },
        {
            'choices'   : (
                (
                    {
                        'token'         : 'crc',
                        'short-help'    : 'Show CRC configuration status',
                    },
                    {
                        'token'         : 'status',
                        'action'        : 'ofad-ctl-command',
                        'command'       : 'crc status',
                        'doc'           : 'forwarding|show-crc-status',
                    },
                ),
                (
                    {
                        'token'         : 'pimu',
                        'short-help'    : 'Show Packet-In Management Unit configuration status',
                    },
                    {
                        'token'         : 'status',
                        'action'        : 'ofad-ctl-command',
                        'command'       : 'pimu status',
                        'doc'           : 'forwarding|show-pimu-status',
                    },
                ),
                (
                    {
                        'token'         : 'pause',
                        'short-help'    : 'Show the port pause configuration status',
                    },
                    {
                        'token'         : 'status',
                        'action'        : 'ofad-ctl-command',
                        'command'       : 'pause status',
                        'doc'           : 'forwarding|show-pause-status',
                    },
                ),
                (
                    {
                        'token'         : 'l2cache',
                        'short-help'    :'Show L2 cache configuration status',
                    },
                    {
                        'token'         : 'status',
                        'action'        : 'ofad-ctl-command',
                        'command'       : 'l2cache status',
                        'doc'           : 'forwarding|show-l2cache-status',
                    },
                ),
            ),
        },

    ),
}

def running_config_forwarding(context, runcfg, words):
    cfg = []
    fwdCfg = ForwardingConfig(OFAgentConfig.forwarding)

    if fwdCfg.isPIMUDisabled():
        cfg.append('forwarding pimu disable\n')
    if not fwdCfg.isL2CACHEDisabled():
        cfg.append('forwarding l2cache enable\n')
    if fwdCfg.isPauseEnabled():
        cfg.append('forwarding pause enable\n')
    if cfg:
        runcfg.append('!\n')
        runcfg += cfg

forwarding_running_config_tuple = (
    (
        {
            'optional'      : False,
            'field'         : 'running-config',
            'type'          : 'enum',
            'values'        : 'forwarding',
            'short-help'    : 'Configuration for forwarding',
            'doc'           : 'running-config|show-forwarding',
        },
    ),
)

run_config.register_running_config('forwarding', 7000, None,
                                   running_config_forwarding,
                                   forwarding_running_config_tuple)
