# Copyright (c) 2013  BigSwitch Networks

import command
import run_config
import error

from sl_util.ofad import OFADConfig, ForwardingConfig

OFAgentConfig = OFADConfig()

def config_forwarding(no_command, data):
    fwdCfg = ForwardingConfig(OFAgentConfig.forwarding)

    # NOTE: this command is negated:
    #       when command is run w/o "no", it's a disable op.
    #       when command is run w/ "no", it's an enable op.
    disabled = not no_command

    if data['type'] == 'crc':
        if disabled:
            print "WARNING: CRC forwarding setting must be consistent on all Big Tap switches in the same forwarding domain."
            fwdCfg.disableCRC()
        else:
            fwdCfg.enableCRC()
    elif data['type'] == 'pimu':
        if disabled:
            fwdCfg.disablePIMU()
        else:
            fwdCfg.enablePIMU()
    else:
        raise error.ActionError('Unspported forwarding config type: %s' % data['type'])

    OFAgentConfig.forwarding = fwdCfg.toJSON()
    OFAgentConfig.write(warn=True)
    OFAgentConfig.reload()

command.add_action('implement-config-forwarding', config_forwarding,
                   {'kwargs': {
                       'no_command' : '$is-no-command',
                       'data'       : '$data',
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
                        'token'         : 'crc',
                        'short-help'    : 'Configure CRC error-checking',
                    },
                    {
                        'token'         : 'disable',
                        'data'          : {'type' : 'crc'},
                        'doc'           : 'forwarding|crc-disable',
                    },
                ),
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
            ),
        },

    ),
}

def running_config_forwarding(context, runcfg, words):
    cfg = []
    fwdCfg = ForwardingConfig(OFAgentConfig.forwarding)

    if fwdCfg.isCRCDisabled():
        cfg.append('forwarding crc disable\n')
    if fwdCfg.isPIMUDisabled():
        cfg.append('forwarding pimu disable\n')

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
