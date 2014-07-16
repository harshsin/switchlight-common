#
#  (c) in 2010-2013 by Big Switch Networks
#

import command

from sl_util import Service, state
from sl_util.ofad import OFADConfig

INTERNAL_SUBMODE_COMMAND_DESCRIPTION = {
    'name'                : '_internal',
    'mode'                : 'config',
    'no-supported'        : False,
    'help'                : 'Enter Internal CLI debugging mode',
    'short-help'          : 'Enter CLI internal debugging mode',
    'doc'                 : 'internal|internal',
    'doc-example'         : 'internal|internal-example',
    'command-type'        : 'config-submode',
    'obj-type'            : None,
    'current-mode-obj-id' : None,
    'parent-field'        : None,
    'submode-name'        : 'config-internal',
    'args'                : (),
}


def lint_action(data):
    words = []
    if 'command' in data:
        words.append(data['command'])
    command.lint_command(words)

command.add_action('lint-action',  lint_action,
                   {'kwargs' : { 'data' : '$data' } } )

INTERNAL_LINT_COMMAND_DESCRIPTION = {
    'name'         : 'lint',
    'mode'         : 'config-internal',
    'no-supported' : False,
    'action'       : 'lint-action',
    'args'         : {
        'optional' : True,
        'field'    : 'command',
        'type'     : 'string',
    }
}


def permute_action(data):
    words = []
    if 'command' in data:
        words.append(data['command'])
    return command.permute_command(words, data.get('qualify'))

command.add_action('permute-action',  permute_action,
                   {'kwargs' : { 'data' : '$data' } } )


INTERNAL_PERMUTE_COMMAND_DESCRIPTION = {
    'name'         : 'permute',
    'mode'         : 'config-internal',
    'no-supported' : False,
    'action'       : 'permute-action',
    'data'         : { 'qualify' : False },
    'args'         : (
        {
            'optional' : True,
            'field'    : 'command',
            'type'     : 'string',
        },
    )
}


INTERNAL_QUALIFY_COMMAND_DESCRIPTION = {
    'name'         : 'qualify',  # berate
    'mode'         : 'config-internal',
    'no-supported' : False,
    'action'       : 'permute-action',
    'data'         : { 'qualify' : True },
    'args'         : (
        {
            'optional' : True,
            'field'    : 'command',
            'type'     : 'string',
        },
    )
}


def save_default_action(data):
    for name, proc in state.get_save_registry():
        print "Running save handler for %s..." % name
        proc()

command.add_action('save-default-action', save_default_action,
                   {'kwargs' : { 'data' : '$data' } } )


INTERNAL_SAVE_DEFAULT_COMMAND_DESCRIPTION = {
    'name'         : 'save-default',
    'mode'         : 'config-internal',
    'no-supported' : False,
    'action'       : 'save-default-action',
    'args'         : (),
}


def revert_default_action(data):
    for name, proc in state.get_revert_registry():
        print "Running revert handler for %s..." % name
        proc()

command.add_action('revert-default-action', revert_default_action,
                   {'kwargs' : { 'data' : '$data' } } )


INTERNAL_REVERT_DEFAULT_COMMAND_DESCRIPTION = {
    'name'         : 'revert-default',
    'mode'         : 'config-internal',
    'no-supported' : False,
    'action'       : 'revert-default-action',
    'args'         : (),
}


def bigdoc_action(data):
    words = []
    if 'command' in data:
        words.append(data['command'])
    return command.get_bigdoc(words)

command.add_action('bigdoc-action',  bigdoc_action,
                   {'kwargs' : { 'data' : '$data' }, } )

INTERNAL_BIGDOC_COMMAND_DESCRIPTION = {
    'name'         : 'bigdoc',
    'mode'         : 'config-internal',
    'no-supported' : False,
    'action'       : 'bigdoc-action',
    'args'         : {
        'optional' : True,
        'field'    : 'command',
        'type'     : 'string',
    }
}

def bigwiki_action(data):
    words = []
    if 'command' in data:
        words.append(data['command'])
    return command.get_bigwiki(words)

command.add_action('bigwiki-action',  bigwiki_action,
                   {'kwargs' : { 'data' : '$data' }, } )

INTERNAL_BIGWIKI_COMMAND_DESCRIPTION = {
    'name'         : 'bigwiki',
    'mode'         : 'config-internal',
    'no-supported' : False,
    'action'       : 'bigwiki-action',
    'args'         : {
        'optional' : True,
        'field'    : 'command',
        'type'     : 'string',
    }

}


#
# FORMATS
#

import fmtcnv


CLI_FORMAT = {
    'cli'  : {
        'field-orderings' : {
            'default' : [
                'version',
                'debug',
                'desc'
                'format',
                'modes',
            ]
        },

        'fields' : {
            'version' : { 'verbose-name' : 'Syntax Version',
                        },
            'debug'   : { 'verbose-name' : 'Debug Level',
                        },
            'desc'    : { 'verbose-name' : 'Desc Modules',
                        },
            'format'  : { 'verbose-name' : 'Format Modules',
                        },
            'modes'   : { 'verbose-name' : 'Submodes',
                        },
        },
    },
}

CLI_MODES_FORMAT = {
    'cli-modes' : {
        'field-orderings' : {
            'default' : [ 'Idx', 'mode', 'command', 'submode',
                        ]
        }
    },
}
