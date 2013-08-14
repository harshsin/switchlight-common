# Copyright (c) 2013  BigSwitch Networks

import command
import run_config
from sl_util import Service


class SSH(Service):
    SVC_NAME = "ssh" 

class Telnet(Service):
    # This is preposterously heavyhanded, but telnet is the only thing
    # we appear to be running out of inetd right now.  If we start using
    # it for the most awesome echo server ever, we'll need to build a
    # useful abstraction class for things that run out of inet.
    SVC_NAME = "openbsd-inetd"
      

def config_ssh(no_command, data):
    if no_command:
        SSH.disable()
    else:
        SSH.enable()

command.add_action('implement-config-ssh', config_ssh,
                    {'kwargs': {
                                 'no_command' : '$is-no-command',
                                 'data'       : '$data',
                               } } )

CONFIG_SSH_COMMAND_DESCRIPTION = {
    'name'         : 'ssh',
    'mode'         : 'config',
    'short-help'   : 'Configure ssh parameters',
    'action'       : 'implement-config-ssh',
    'no-action'    : 'implement-config-ssh',
    'doc'          : 'access|ssh',
    'doc-example'  : 'access|ssh-example',
    'args'         : (
        {
            'token'           : 'enable',
            'short-help'      : 'Enable or disable ssh server',
        },
    )
}


def config_telnet(no_command, data):
    if no_command:
        Telnet.disable()
    else:
        Telnet.enable()

command.add_action('implement-config-telnet', config_telnet,
                    {'kwargs': {
                                 'no_command' : '$is-no-command',
                                 'data'       : '$data',
                               } } )

CONFIG_TELNET_COMMAND_DESCRIPTION = {
    'name'         : 'telnet',
    'mode'         : 'config',
    'short-help'   : 'Configure telnet parameters',
    'action'       : 'implement-config-telnet',
    'no-action'    : 'implement-config-telnet',
    'doc'          : 'access|telnet',
    'doc-example'  : 'access|telnet-example',
    'args'         : (
        {
            'token'           : 'enable',
            'short-help'      : 'Enable or disable telnet server',
        },
    )
}


def running_config_ssh(context, runcfg, words):
    # collect component-specific config    
    # in this case, determine if sshd is running
    # and if so, comp_runcfg.append('ssh enable\n')

    runcfg.append("!\n")
    if SSH.status() == Service.RUNNING:
        runcfg.append("ssh enable\n")
    else:
        runcfg.append("no ssh enable\n")


ssh_running_config_tuple = (
    (
        {
            'optional'   : False,
            'field'      : 'running-config',
            'type'       : 'enum',
            'values'     : 'ssh',
            'short-help' : 'Configuration for ssh',
            'doc'        : 'running-config|show-ssh',
        },
    ),
)

run_config.register_running_config('ssh', 2000,  None,
                                   running_config_ssh,
                                   ssh_running_config_tuple)

def running_config_telnet(context, runcfg, words):
    runcfg.append("!\n")
    if Telnet.status() == Service.RUNNING:
        runcfg.append("telnet enable\n")
    else:
        runcfg.append("no telnet enable\n")

telnet_running_config_tuple = (
    (
        {
            'optional'   : False,
            'field'      : 'running-config',
            'type'       : 'enum',
            'values'     : 'telnet',
            'short-help' : 'Configuration for telnet',
            'doc'        : 'running-config|show-telnet',
        },
    ),
)

run_config.register_running_config('telnet', 2100,  None,
                                   running_config_telnet,
                                   telnet_running_config_tuple)

