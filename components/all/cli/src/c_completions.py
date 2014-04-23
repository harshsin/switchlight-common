#
# Copyright (c) 2011-2014 Big Switch Networks, Inc.
# All rights reserved.
#

import os
import re
import error
import command
import collections
import utif


def pretty(text):
    """
    For object-type's, remove dashes, capitalize first character
    """
    return text.replace('-', ' ').capitalize()


#
# COMPLETION PROCS
#
# 'completions' is a dictionary, where the keys are the actual text
# of the completion, while the value is the reason why this text 
# was added.  The 'reason' provides the text for the two-column
# help printed for the '?' character.
#

def complete_config(prefix, data, completions, copy = False):
    """
    Complete selections for the 'copy' command.
    """

    configs = bigsh.store.get_user_data_table('', "latest")

    # exclude source if its in the data
    source = data.get('source','')
    src_dst = 'source' if source == '' else 'destination'

    any = False
    any_config = False

    if copy:
        if 'running-config'.startswith(prefix):
            if source != 'running-config':
                completions['running-config '] = 'running-config %s' % src_dst

    for c in configs:
        if ('config://' + c['name']).startswith(prefix):
            if source != "config://" + c['name']:
                completions["config://" + c['name'] + ' '] = \
                    'Saved Configuration %s' % src_dst
            any_config = True

    if source != '' and 'config://'.startswith(prefix):
        completions['config://'] = 'config prefix %s' % src_dst

    if copy:
        for additions in ["http://", "file://", "ftp://", "tftp://", 'config://' ]:
            if additions.startswith(prefix):
                completions[additions] = 'other %s' % src_dst


def complete_description_versions(prefix, completions):
    for element in os.listdir(bigsh.command_packages_path()):
        if element.startswith('__init__.py'):
            pass
        elif element.startswith('version'):
            # len('element') -> 7
            version = "%2.2f" % (float(element[7:]) / 100)
            if version[-2:] == '00':
                version = version[:2] + '0'
            if version.startswith(prefix):
                completions[version] = 'VERSION'
            if version == '2.0':    # currently if 2.0 exists, so does 1.0
                if '1.0'.startswith(prefix):
                    completions['1.0'] = 'VERSION'
        else:
            if element.startswith(prefix):
                completions[element] = 'VERSION'

def init_completions(bs):
    global bigsh
    bigsh = bs

    command.add_completion('complete-config', complete_config,
                           {'kwargs': {'prefix': '$text',
                                       'data': '$data',
                                       'completions': '$completions'}})

    command.add_completion('complete-config-copy', complete_config,
                           {'kwargs': {'prefix': '$text',
                                       'data': '$data',
                                       'completions': '$completions',
                                       'copy' : True }})

    command.add_completion('description-versions', complete_description_versions,
                           {'kwargs': {'prefix': '$text',
                                       'completions': '$completions'}})
