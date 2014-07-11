#
#  bigsh - The BigSwitch Controller Shell
#  (c) in 2012 by Big Switch Networks
#  All rights reserved
#

#
# show running-config
#  and associated
#

import time
import re
import socket
import traceback
import utif

from sl_util import const

def init_running_config(bs):
    global bigsh
    bigsh = bs


running_config_registry = {}

running_config_command_choices = {
    'optional' : True,
    'choices'  : (
    )
}

#
# --------------------------------------------------------------------------------

def register_running_config(name, order, feature, running_config_proc, command_tuple = None):
    """
    Register a callback to manage the display of component running configs

    @feature a predicate to call, returns True/False to enable/disable this entry
    """
    running_config_registry[name] = { 'order'   : order,
                                      'feature' : feature,
                                      'proc'    : running_config_proc }
    if command_tuple:
        global running_config_command_choices
        running_config_command_choices['choices'] += command_tuple

#
# --------------------------------------------------------------------------------

def registry_items_enabled():
    """
    Return a list of active running config entries, this is a subset of
    the registered items, only items which are currently enabled via features
    """
    return [name for name in running_config_registry.keys()
            if running_config_registry[name]['feature'] == None or
               running_config_registry[name]['feature'](bigsh) == True]


#
# --------------------------------------------------------------------------------

def perform_running_config(name, context, config, words):
    """
    Callout to append to config
    """
    if name in running_config_registry:
        try:
            running_config_registry[name]['proc'](context, config, words)
        except Exception, e:
            if bigsh.debug or bigsh.debug_backtrace:
                traceback.print_exc()
            else:
                config.append("! ### ERROR: Cannot get running-config for '%s'!\n" % (name))


#
# --------------------------------------------------------------------------------

def implement_show_running_config(words):
    """
    Manager for the 'show running-config' command, which calls the
    specific detail functions for any of the parameters.
    """

    # LOOK! hardwired - need to use the obj_type_info and the field_list
    # LOOK! how are these sorted?
    config = []
    if len(words) > 0:
        # pick the word
        choice = utif.full_word_from_choices(words[0],
                                             registry_items_enabled())
        if choice: 
            perform_running_config(choice, bigsh, config, words)
        else:
            return bigsh.error_msg("unknown running-config item: %s" % words[0])
        # config[:-1] removes the last trailing newline
        return ''.join(config)[:-1]
    else:
        # Create the order based on the registration value
        running_config_order = sorted(registry_items_enabled(),
                                      key=lambda item: running_config_registry[item]['order'])
        exclude_list=[]
        for rc in running_config_order:
            if rc not in exclude_list:
                perform_running_config(rc, bigsh, config, words)

        prefix = []
        if len(config) > 0:
            prefix.append("!\n")
            prefix.append("! hostname: %s\n" % socket.gethostname())
            try:
                prefix.append("! version: %s\n" % \
                              file(const.VERSION_PATH).read().strip())
            except IOError:
                pass
            prefix.append("! current time: %s\n" % time.strftime(
                    "%Y-%m-%d %H:%M:%S %Z", time.localtime()))
            prefix.append("!\n")

        # config[:-1] removes the last trailing newline
        return ''.join(prefix)  + ''.join(config)[:-1]


