# Copyright (c) 2014  BigSwitch Networks

import command

from sl_util import const
from sl_util.ofad import OFADCtl

def show_tunnel(data):
    cmd = 'tunnel summary'
    if 'tunnel-id' in data:
        cmd += ' %s%d' % (const.TUN_PORT_BASE, data['tunnel-id'])
    OFADCtl.run(cmd)

command.add_action('implement-show-tunnel', show_tunnel,
                   {'kwargs': {'data' : '$data',}})

SHOW_TUNNEL_COMMAND_DESCRIPTION = {
    'name'          : 'show',
    'mode'          : 'login',
    'action'        : 'implement-show-tunnel',
    'doc'           : 'tunnel|show',
    'doc-example'   : 'tunnel|show-example',
    'no-supported'  : False,
    'args'          : (
        {
            'token'         : 'tunnel',
            'short-help'    : 'Show tunnel summary',
        },
        {
            'field'         : 'tunnel-id',
            'base-type'     : 'integer',
            # FIXME: Perform range check. Ideally at some point we can
            #        get the tun min and max from ofad.
            'syntax-help'   : 'Tunnel ID',
            'optional'      : True,
        },
    ),
}
