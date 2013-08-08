#
# Copyright (c) 2011,2012 Big Switch Networks, Inc.
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


def complete_staticflow_actions(prefix, data, completions):
    # peek at the last character in the prefix:
    #  if it's a comma, then choose all the possible actions
    #  if its a equal, then display the choices for this option
    
    prefix_parts = []

    actions = {
        'output='            : 'Describe packet forwarding',
        'enqueue='           : 'Enqueue packet',
        'strip-vlan='        : 'Strip Vlan',
        'set-vlan-id='       : 'Set Vlan',
        'set-vlan-priority=' : 'Set Priority',
        'set-src-mac='       : 'Set Src Mac',
        'set-dst-mac='       : 'Set Dst Mac',
        'set-tos-bits='      : 'Set TOS Bits',
        'set-src-ip='        : 'Set IP Src',
        'set-dst-ip='        : 'Set IP Dst',
        'set-src-port='      : 'Set Src IP Port',
        'set-dst-port='      : 'Set dst IP Port',
    }

    action_choices = {
        ('output=', 'all')          : 'Forward to all ports',
        ('output=', 'controller')   : 'Forward to controller',
        ('output=', 'local')        : 'Forward to local',
        ('output=', 'ingress-port') : 'Forward to ingress port',
        ('output=', 'normal')       : 'Forward to ingress port',
        ('output=', 'flood')        : 'Forward, flood ports',
        ('output=', ('<number>', '<number>'))  : 'Forward, to a specific port',

        ('enqueue=', ('<portNumber>.<queueID>', '<portNumber>.<queueID>')) : 'Enqueue to port, queue id',

        ('set-vlan-id=',('<vlan number>','<vlan number>')) : 'Set vlan to <vlan number>',
        
        ('set-vlan-priority=',('<vlan prio>','<vlan prio>')) : 'Set vlan priority to <prio>',

        ('set-tos-bits=',('<number>',)) : 'Set TOS bits',
        ('set-src-mac=',('<src-mac-address>',)) : 'Set src mac address',

        ('set-dst-mac=',('<dst-mac-address>',)) : 'Set dst mac address',
        
        ('set-src-ip=',('<src-ip-address>',)) : 'Set src mac address',
        
        ('set-dst-ip=',('<src-ip-address>',)) : 'Set dst ip address',
    }

    for ps in prefix.split(','):
        ps_parts = ps.split('=')
        if len(ps_parts) == 1 and ps_parts[0] != '':
            # possibly incomplete item before the '='
            for choice in [x for x in actions.keys() if x.startswith(ps_parts[0])]:
                completions[choice] = actions[choice]
            return
        elif len(ps_parts) == 2:
            if len(ps_parts[0]) and len(ps_parts[1]):
                prefix_parts.append((ps_parts[0], ps_parts[1]))
            elif len(ps_parts[0]) and len(ps_parts[1]) == 0:
                prefix_parts.append((ps_parts[0], ))

    if prefix == '' or prefix.endswith(','):
        completions.update(actions)
    elif prefix.endswith('='):
        last = prefix_parts[-1]
        for ((match, next), desc) in action_choices.items():
            if match[:-1] != last[0]:
                continue
            if type(next) == str:
                completions[match + next] = desc
            elif type(next) == tuple:
                completions[(match + next[0], match + next[0])] = desc
            # else?  display error?
    elif len(prefix_parts):
        last = prefix_parts[-1]
        if len(last) == 1:
            pass
        elif len(last) == 2:
            # try to find the left item
            for ((match, next), desc) in action_choices.items():
                if match[:-1] != last[0]:
                    continue
                if type(next) == str and next == last[1]:
                    eol = prefix + ' <cr>'
                    completions[(eol, eol)] = 'Complete Choice'
                    another = prefix + ','
                    completions[(another, another)] = 'Add another action'
                elif type(next) == str and next.startswith(last[1]):
                    base_part = ''.join(prefix.rpartition(',')[:-1])
                    completions[base_part + last[0] + '=' + next] = 'Complete selection'
                elif len(last[1]):
                    # hard to say what choices can be added here,
                    # there are some characters after '=', but none
                    # which match some prefix.
                    pass

                # how to match the values?


def complete_description_versions(prefix, completions):
    for element in os.listdir(bigsh.command_packages_path()):
        if element == '__init__.py':
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

    command.add_completion('complete-staticflow-actions', complete_staticflow_actions,
                           {'kwargs': {'prefix': '$text',
                                       'data': '$data',
                                       'completions': '$completions'}})

    command.add_completion('description-versions', complete_description_versions,
                           {'kwargs': {'prefix': '$text',
                                       'completions': '$completions'}})
