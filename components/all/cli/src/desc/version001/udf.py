# Copyright (c) 2014  BigSwitch Networks

SHOW_UDF_COMMAND_DESCRIPTION = {
    'name'          : 'show',
    'mode'          : 'login',
    'no-supported'  : False,
    'args'          : (
        {
            'token'         : 'udf',
            'action'        : 'ofad-ctl-command',
            'command'       : 'udf-show',
            'short-help'    : 'Show UDF configurations',
            'doc'           : 'udf|show',
        },
    )
}
