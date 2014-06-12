# Copyright (c) 2013  BigSwitch Networks

import command
import error
import utif
import run_config

from sl_util.ofad import OFADConfig, OFADCtl, PortManager
from sl_util import utils
from sl_util import const

from switchlight.platform.current import SwitchLightPlatform
from switchlight.platform.base import *

Platform=SwitchLightPlatform()
OFAgentConfig = OFADConfig()
PortManager.setPhysicalBase(OFAgentConfig.physical_base_name)
PortManager.setLAGBase(OFAgentConfig.lag_base_name)

def config_port_channel(no_command, data, is_init):
    portManager = PortManager(OFAgentConfig.port_list)

    portId = data["port-channel-id"]

    componentPorts = None
    if "interface-list" in data:
        componentPorts = utif.resolve_port_list(data["interface-list"])
        lagCompMax = Platform.platinfo.LAG_COMPONENT_MAX;
        if lagCompMax is None:
            raise error.ActionError("Cannot determine max number of component ports supported")
        if len(componentPorts) > lagCompMax:
            raise error.ActionError("Too many component ports. Max supported is %d" % lagCompMax)

        for p in componentPorts:
            portManager.checkValidPhysicalPort(p)

    hashType = data.get("hash-type", None)

    if no_command:
        portManager.unconfigureLAGPort(int(portId), componentPorts,
                                       is_init=is_init)
    else:
        portManager.configureLAGPort(int(portId), componentPorts, hashType,
                                     is_init=is_init)

    OFAgentConfig.port_list = portManager.toJSON()
    OFAgentConfig.write(warn=True)
    OFAgentConfig.reload(deferred=is_init)

command.add_action('implement-config-port-channel', config_port_channel,
                   {'kwargs': {
                       'no_command' : '$is-no-command',
                       'data'       : '$data',
                       'is_init'    : '$is-init',
                    }})

CONFIG_PORTCHANNEL_COMMAND_DESCRIPTION = {
    'name'          : 'port-channel',
    'mode'          : 'config',
    'short-help'    : 'Configure Port Channel Settings',
    'action'        : 'implement-config-port-channel',
    'no-action'     : 'implement-config-port-channel',
    'doc'           : 'port-channel|port-channel',
    'doc-example'   : 'port-channel|port-channel-example',
    'args'          : (
        {
            'field'             : 'port-channel-id',
            'base-type'         : 'integer',
            'range'             : (const.LAG_MIN_NUM, const.LAG_MAX_NUM),
            'syntax-help'       : 'Port channel ID',
        },
        {
            'field'             : 'interface-list',
            'tag'               : 'interface-list',
            'short-help'        : 'Interface port range list',
            'syntax-help'       : 'Comma-separated list of integers or integer ranges',
            'type'              : 'string',
            'doc'               : 'port-channel|interface-list',
            'optional'          : False,
            'optional-for-no'   : True,
        },
        {
            'field'             : 'hash-type',
            'tag'               : 'hash',
            'short-help'        : 'Hash type for port channel',
            'type'              : 'enum',
            # NOTE: See PAN-480. hash-max does not work.
            'values'            : ('L2', 'L3'),
            # FIXME: syntax-help for each value doesn't work yet
            'doc'               : 'port-channel|+',
            'optional'          : True,
            'optional-for-no'   : True,
        },
    ),
}

def show_port_channel(data):
    portManager = PortManager(OFAgentConfig.port_list)

    portId = data["port-channel-id"]

    lagName = PortManager.getLAGName(portId)
    if lagName not in portManager.getExistingPorts():
        raise error.ActionError("%s is not an existing interface" % lagName)

    OFADCtl.run("port %s" % lagName)

command.add_action('implement-show-port-channel', show_port_channel,
                   {'kwargs': {'data' : '$data',}})

SHOW_PORTCHANNEL_COMMAND_DESCRIPTION = {
    'name'          : 'show',
    'mode'          : 'login',
    'action'        : 'implement-show-port-channel',
    'doc'           : 'port-channel|show',
    'doc-example'   : 'port-channel|show-example',
    'no-supported'  : False,
    'args'          : (
        {
            'token'         : 'port-channel',
            'short-help'    : 'Show Port Channel statistics',
        },
        {
            'field'         : 'port-channel-id',
            'base-type'     : 'integer',
            'range'         : (const.LAG_MIN_NUM, const.LAG_MAX_NUM),
            'syntax-help'   : 'Port channel ID',
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
            'optional'      : False,
            'field'         : 'running-config',
            'type'          : 'enum',
            'values'        : 'port-channel',
            'short-help'    : 'Configuration for port channel interfaces',
            'doc'           : 'running-config|show-port-channel',
        },
    ),
)

run_config.register_running_config('port-channel', 6000, None,
                                   running_config_port_channel,
                                   port_channel_running_config_tuple)
