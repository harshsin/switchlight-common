# Copyright (c) 2013  BigSwitch Networks

import command
import error
import utif
import run_config

from sl_util.ofad import OFADConfig, PortManager

OFAgentConfig = OFADConfig()
PortManager.setPhysicalBase(OFAgentConfig.physical_base_name)
PortManager.setLAGBase(OFAgentConfig.lag_base_name)

LAG_MIN_NUM = 1
LAG_MAX_NUM = 30

def config_port_channel(no_command, data):
    portManager = PortManager(OFAgentConfig.port_list)

    portId = data["port-channel-id"]
    if portId < LAG_MIN_NUM or portId > LAG_MAX_NUM:
        raise error.ActionError("Port channel ID must be from %d to %d" % \
                                (LAG_MIN_NUM, LAG_MAX_NUM))

    componentPorts = None
    if "interface-list" in data:
        componentPorts = utif.resolve_port_list(data["interface-list"])
        if componentPorts is None:
            raise error.ActionError("Invalid interface list spec: %s" % data["interface-list"])

        for p in componentPorts:
            portManager.checkValidPhysicalPort(p)

    hashType = data.get("hash-type", None)

    if no_command:
        portManager.unconfigureLAGPort(int(portId), componentPorts)
    else:
        portManager.configureLAGPort(int(portId), componentPorts, hashType)

    OFAgentConfig.port_list = portManager.toJSON()
    OFAgentConfig.write(warn=True)
    OFAgentConfig.reload()

command.add_action('implement-config-port-channel', config_port_channel,
                   {'kwargs': {
                       'no_command' : '$is-no-command',
                       'data'       : '$data',
                    }})

# FIXME: add documentation for port-channel commands
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
            'field'             : 'interface-list',
            # FIXME: type 'interface-list' doesn't work
            'tag'               : 'interface-list',
            'short-help'        : 'Interface port range list',
            'type'              : 'string',
            'optional'          : False,
            'optional-for-no'   : True,
        },
        {
            'field'             : 'hash-type',
            'tag'               : 'hash',
            'short-help'        : 'Hash type for port channel',
            'type'              : 'enum',
            'values'            : ('L2', 'L3', 'MAX'),
            'optional'          : True,
            'optional-for-no'   : True,
        },
    ),
}

def running_config_port_channel(context, runcfg, words):
    portManager = PortManager(OFAgentConfig.port_list)

    lagList = [(PortManager.getLAGId(lag.portName), lag) for lag in portManager.getLAGs()]
    cfg = []
    for portId, lag in sorted(lagList, key=lambda x: x[0]):
        interfaceList = ",".join([str(p) for p in lag.componentPorts])
        line = "port-channel %d interface-list %s" % (portId, interfaceList)
        if lag.hash:
            line += " hash %s" % lag.hash
        cfg.append("%s\n" % line)

    if cfg:
        runcfg.append("!\n")
        runcfg += cfg

port_channel_running_config_tuple = (
    (
        {
            'optional'  : False,
            'field'     : 'running-config',
            'type'      : 'enum',
            'values'    : 'port-channel',
            # FIXME: add short-help and doc
        },
    ),
)

run_config.register_running_config('port-channel', 6000, None,
                                   running_config_port_channel,
                                   port_channel_running_config_tuple)
