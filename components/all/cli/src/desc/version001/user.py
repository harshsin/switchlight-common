# Copyright (c) 2013  BigSwitch Networks

import command
import run_config
import error

import os
import shutil
import string
import random
import re
import crypt
import spwd

import subprocess
from sl_util import shell, const, conf_state


#
# These are the available user accounts we can configure.
# We only modify accounts, we don't create them, so
# they must be preexisting.
#
# This will obviously need to change if we allow
# user creation in the future.
#
CONFIG_USERS = ('admin', 'recovery')

def gen_salt():
    # return an eight character salt value
    salt_map = './' + string.digits + string.ascii_uppercase + \
        string.ascii_lowercase
    rand = random.SystemRandom()
    salt = ''
    for i in range(0, 8):
        salt += salt_map[rand.randint(0, 63)]
    return salt


def config_user(no_command, data):
    # FIXME handle adding or removing a user
    if data['username'] not in CONFIG_USERS:
        raise error.ActionError('Only the %s user can be configured.' %
                                (', '.join(CONFIG_USERS[0:-1]) + " or " + CONFIG_USERS[-1]))

    if no_command:
        hashedpw = spwd.getspnam(data['username'])[1]
        if 'password' in data:
            if 'hashed' in data:
                hashedinput = data['password']
            else:
                hashedinput = crypt.crypt(data['password'], hashedpw)
            if hashedinput != hashedpw:
                raise error.ActionError('Passwords do not match')
        # extra quotes so shell.call will have a password value of ''
        pw = "''"
    else:
        if 'hashed' in data:
            pw = data['password']
        else:
            # 1 is MD5
            pw = crypt.crypt(data['password'], '$1$%s$' % gen_salt())

    # escape $ to prevent shell variable substitution
    pw = pw.replace('$', '\$')

    try:
        shell.call('usermod %s --password %s' % (data['username'], pw))
    except subprocess.CalledProcessError:
        raise error.ActionError('Unable to set password for user %s' %
                                data['username'])

command.add_action('implement-config-user', config_user,
                    {'kwargs': {
                                 'no_command' : '$is-no-command',
                                 'data'       : '$data',
                               } } )

CONFIG_USER_COMMAND_DESCRIPTION = {
    'name'         : 'username',
    'mode'         : 'config',
    'short-help'   : 'Configure username and password',
    'action'       : 'implement-config-user',
    'no-action'    : 'implement-config-user',
    'doc'          : 'user|username',
    'doc-example'  : 'user|username-example',
    'args'         : (
        {
            'field'           : 'username',
            'type'            : 'identifier',
            'optional-for-no' : False,
        },
        {
            'optional-for-no' : True,
            'args' : (
                {
                    'optional-for-no' : False,
                    'choices' : (
                        (
                            {
                                'token'           : 'secret',
                                'short-help'      : 'Password for this user',
                            },
                        ),
                        (
                            {
                                'token'           : 'password',
                                'short-help'      : 'Password for this user',
                            },
                        ),
                    ),
                },
                {
                    'choices' : (
                        (
                            {
                                'field'           : 'password',
                                'base-type'       : 'string-reserved',
                                'reserved'        : [ 'md5', 'hash' ],
                                'syntax-help'     : 'Plaintext password',
                            },
                        ),
                        (
                            {
                                'field'           : 'hashed',
                                'type'            : 'enum',
                                'values'          : ( 'md5', 'hash' ),
                                'short-help'      : 'Secret hash type',
                            },
                            {
                                'field'           : 'password',
                                'type'            : 'hashed-password',
                            },
                        ),
                    ),
                },
            ),
        },
    )
}


def save_default_user_passwords():
    ws = os.path.join(const.DEFAULT_DIR, 'user')
    if os.path.exists(ws):
        print 'Default user passwords already exist.'
        return

    os.makedirs(ws)
    dst = os.path.join(ws, os.path.basename(const.USER_PWD_PATH))
    shutil.copy(const.USER_PWD_PATH, dst)

def revert_default_user_passwords():
    ws = os.path.join(const.DEFAULT_DIR, 'user')
    if not os.path.exists(ws):
        print 'Default user passwords do not exist.'
        return

    src = os.path.join(ws, os.path.basename(const.USER_PWD_PATH))
    shutil.copy(src, const.USER_PWD_PATH)

conf_state.register_save('user', save_default_user_passwords)
conf_state.register_revert('user', revert_default_user_passwords)


def running_config_username(context, runcfg, words):
    comp_runcfg = []

    # collect component-specific config
    for user in CONFIG_USERS:
        hashedpw = spwd.getspnam(user)[1]
        if len(hashedpw) > 0:
            comp_runcfg.append('username %s secret hash %s\n' %
                               (user, hashedpw))

    # attach component-specific config
    if len(comp_runcfg) > 0:
        runcfg.append('!\n')
        runcfg += comp_runcfg


username_running_config_tuple = (
    (
        {
            'optional'   : False,
            'field'      : 'running-config',
            'type'       : 'enum',
            'values'     : 'username',
            'short-help' : 'Configuration for username',
            'doc'        : 'running-config|show-username',
        },
    ),
)

run_config.register_running_config('username', 1500,  None,
                                   running_config_username,
                                   username_running_config_tuple)

