#
# Copyright (c) 2011,2012 Big Switch Networks, Inc.
# All rights reserved.
#

import re
import numbers
import collections
import traceback
import types
import json
import time
import sys
import datetime
import os
import subprocess
import socket

import error
import command
import run_config

#
# ACTION PROCS
#

def push_mode_stack(mode_name, data, create=True):
    """
    Push a submode on the config stack.
    """
    global bigsh

    # Some few minor validations: enable only in login, config only in enable,
    # and additional config modes must also have the same prefix as the
    # current mode.
    current_mode = bigsh.current_mode()

    if bigsh.description:   # description debugging
        print "push_mode: ", mode_name, data

    # See if this is a nested submode, or whether some current modes
    # need to be popped.
    if (mode_name.startswith('config-') and 
      (not mode_name.startswith(current_mode) or (mode_name == current_mode))):

        bigsh.pop_mode()
        current_mode = bigsh.current_mode()
        # pop until it it matches
        while not mode_name.startswith(current_mode):
            if len(bigsh.mode_stack) == 0:
                raise error.CommandSemanticError('%s not valid within %s mode' %
                                           (mode_name, current_mode))
            bigsh.pop_mode()
            current_mode = bigsh.current_mode()

    elif mode_name in ['config', 'enable', 'login']:
        # see if the mode is in the stack
        if mode_name in [x['mode_name'] for x in bigsh.mode_stack]:
            if bigsh.description:   # description debugging
                print 'push_mode: popping stack for', mode_name
            current_mode = bigsh.current_mode()
            while current_mode != mode_name:
                bigsh.pop_mode()
                current_mode = bigsh.current_mode()
            return

    if bigsh.description:   # description debugging
        print "push_mode: ", mode_name
    bigsh.push_mode(mode_name)

def pop_mode_stack():
    global bigsh

    if bigsh.description:   # description debugging
        print "pop_mode: "
    bigsh.pop_mode()

# KHC FIXME remove?
def command_version(data):
    """
    The version command will later manage changing the syntax to match
    the requested version.
    """
    new_version = data.get('version')
    if new_version == None:
        return

    version = new_version # save for error message
    new_version = bigsh.desc_version_to_path_elem(new_version)

    # skip version change is this is the current version.
    if bigsh.desc_version == new_version:
        return

    # see if the requested version exists
    if not bigsh.command_packages_exists(new_version):
        print 'No command description group for version %s' % version
        return

    # run 'env [envriron_vars] ... bigcli.py'
    command = ['env']
    command.append('BIGCLI_COMMAND_VERSION=%s' % version)
    command.append('BIGCLI_STARTING_MODE=config')
    if os.path.exists('/opt/bigswitch/cli/bin/bigcli'):
        # controller VM
        command.append('/opt/bigswitch/cli/bin/bigcli --init')
    else:
        # developer setup
        base = os.path.dirname(__file__)
        command.append(os.path.join(base, 'bigcli.py'))
        command.append('--init')

    # dump the command descriptions, and read a new set.
    # open a subshell with a new command version
    subprocess.call(command, cwd=os.environ.get("HOME"))

    return

def command_clearterm():
    """
    Print reset characters to the screen to clear the console
    """
    subprocess.call("reset")

def command_cli_variables_set(variable, value, data):
    global bigsh

    if variable == 'debug':
        print '***** %s cli debug *****' % \
                ('Enabled' if value else 'Disabled')
        bigsh.debug = value
    elif variable == 'cli-backtrace':
        print '***** %s cli debug backtrace *****' % \
                ('Enabled' if value else 'Disabled')
        bigsh.debug_backtrace = value
    elif variable == 'cli-batch':
        print '***** %s cli batch mode *****' % \
                ('Enabled' if value else 'Disabled')
        bigsh.batch = value
    elif variable == 'description':
        print '***** %s command description mode *****' % \
                ('Enabled' if value else 'Disabled')
        bigsh.description = value
    elif variable == 'rest':
        if 'record' in data and value:
            print '***** Eanbled rest record mode %s *****' % \
                (data['record'])
            url_cache.record(data['record'])
            return
        print '***** %s display rest mode *****' % \
                ('Enabled' if value else 'Disabled')
        if 'detail' in data and data['detail'] == 'details':
            if value == True:
                bigsh.disply_rest_detail = value
                bigsh.store.display_reply_mode(value)
        bigsh.display_rest = value
        bigsh.store.display_mode(value)
        if value == False:
            bigsh.disply_rest_detail = value
            bigsh.store.display_reply_mode(value)
            url_cache.record(None)
    elif variable == 'set':
        if 'length' in data:
            bigsh.length = utif.try_int(data['length'])


def command_cli_set(variable, data):
    command_cli_variables_set(variable, True, data)

def command_cli_unset(variable, data):
    command_cli_variables_set(variable, False, data)


def command_shell_command(script):

    def shell(args):
        subprocess.call(["env", "SHELL=/bin/bash", "/bin/bash"] + list(args),
                        cwd=os.environ.get("HOME"))
        print

    print "\n***** Warning: this is a debug command - use caution! *****"
    if script == 'bash':
        print '***** Type "exit" or Ctrl-D to return to the Switch Light CLI *****\n'
        shell(["-l", "-i"])
    elif script == 'python':
        print '***** Type "exit()" or Ctrl-D to return to the Switch Light CLI *****\n'
        shell(["-l", "-c", "python"])
    else:
        # XXX possibly run the script directly?
        print "Unknown debug choice %s" % script


def command_prompt_update():
    """
    Action to recompute the prompt, used when there's some possibility
    the prompt has changes after some other action (hostname update, for example)
    """
    bigsh.update_prompt()

#
# Initialize action functions 
#
#

def init_actions(bs):
    global bigsh
    bigsh = bs

    command.add_action('push-mode-stack', push_mode_stack,
                       {'kwargs': {'mode_name': '$submode-name',
                                   'data': '$data',
                                   'create': '$create'}})

    command.add_action('pop-mode-stack', pop_mode_stack)

    command.add_action('version',  command_version,
                        {'kwargs': {'data' : '$data',
                                   }})

    command.add_action('clearterm',  command_clearterm)

    command.add_action('cli-set', command_cli_set,
                        {'kwargs': {'variable' : '$variable',
                                    'data'     : '$data',
                                   }})

    command.add_action('cli-unset', command_cli_unset,
                        {'kwargs': {'variable' : '$variable',
                                    'data'     : '$data',
                                   }})

    command.add_action('shell-command', command_shell_command,
                        {'kwargs': {'script' : '$command',
                                   }})
    
    command.add_action('prompt-update', command_prompt_update,)

