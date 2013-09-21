# Copyright (c) 2013  BigSwitch Networks

import command
import error
from sl_util.ofad import OFADConfig, LAGPort

OFAgentConfig = OFADConfig()

LAG_PORT_NUM_BASE = 60
LAG_MIN_NUM = 1
LAG_MAX_NUM = 30

def config_port_channel(no_command, data):
    portListDict = OFAgentConfig.port_list
    lagName = "lag%d" % data["port-channel-id"]
    componentPorts = []
    if "interface-list" in data:
        componentPorts = ([int(p) for p in data["interface-list"].split(",")])

    if no_command:
        if lagName not in portListDict:
            return

        if len(componentPorts) != 0 and componentPorts != portListDict[lagName]["component_ports"]:
            raise error.ActionError("Interface list specified does not match the existing list")

        portListDict.pop(lagName)
    else:
        portNumber = LAG_PORT_NUM_BASE + int(data["port-channel-id"])
        lag = LAGPort(lagName)
        lag.setComponentPorts(componentPorts)
        lag.setPortNumber(portNumber)
        portListDict[lagName] = lag.toJSON()

    OFAgentConfig.port_list = portListDict
    OFAgentConfig.write(warn=True)
    OFAgentConfig.reloadService()

command.add_action('implement-config-port-channel', config_port_channel,
                   {'kwargs': {
                       'no_command' : '$is-no-command',
                       'data'       : '$data',
                    }})

CONFIG_PORTCHANNEL_COMMAND_DESCRIPTION = {
    'name'          : 'port-channel',
    'mode'          : 'config',
    'short-help'    : 'Configure Port Channel Settings',
    'action'        : 'implement-config-port-channel',
    'no-action'     : 'implement-config-port-channel',
    'args'          : (
        {
            'field'             : 'port-channel-id',
            'type'              : 'integer',
            'syntax-help'       : 'Port channel ID',
        },
        {
            'choices'           : (
                (
                    {
                        'token'             : 'interface-list',
                        'short-help'        : 'Interface port range list',
                        'optional'          : False,
                        'optional-for-no'   : True,
                    },
                    {
                        'field'             : 'interface-list',
                        # FIXME: type 'interface-list' doesn't work
                        'type'              : 'string',
                        'optional'          : False,
                        'optional-for-no'   : True,
                    },
                ),
            ),
        },
    ),
}
