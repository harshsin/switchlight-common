# Copyright (c) 2013  BigSwitch Networks

SHOW_INVENTORY_COMMAND_DESCRIPTION = {
    'name'          : 'show',
    'mode'          : 'login',
    'no-supported'  : False,
    'args'          : (
        {
            'token'         : 'inventory',
            'action'        : 'ofad-ctl-command',
            'command'       : 'inventory',
            'short-help'    : 'Show SFP inventory',
            'doc'           : 'inventory|show',
        },
    )
}
