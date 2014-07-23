# Copyright (c) 2014  BigSwitch Networks

SHOW_TUNNEL_COMMAND_DESCRIPTION = {
    'name'          : 'show',
    'mode'          : 'login',
    'no-supported'  : False,
    'args'          : (
        {
            'token'         : 'tunnel',
            'action'        : 'ofad-ctl-command',
            'command'       : 'tunnel summary',
            'short-help'    : 'Show tunnel summary',
            'doc'           : 'tunnel|show',
        },
    ),
}
