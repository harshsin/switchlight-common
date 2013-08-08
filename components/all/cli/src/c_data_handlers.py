#
# Copyright (c) 2011,2012 Big Switch Networks, Inc.
# All rights reserved.
#

#
# DATA HANDLERS
#

import re
import utif
import error
import command
import time
import datetime
import traceback


COMMAND_CIDR_RE = re.compile(r'^((\d{1,3}\.){3}\d{1,3})/(\d{1,2}?)$')


def split_cidr_data_handler(value, data,
                            dest_ip='ip', dest_netmask='netmask', neg = False):
    """
    Split a cidr address (e.g. 192.168.1.1/24) into separate IP address
    and netmask value. The names of the ip and netmask fields are
    specified (typically directly in the same block/dictionary where
    the argument data handler is specifed) with a 'dest-ip' and a
    'dest-netmask' values.
    """
    global bigsh

    m = COMMAND_CIDR_RE.match(value)
    if m:
        bits = int(m.group(3))
        if bits > 32:
            raise error.ArgumentValidationError("max cidr block is 32")

        data[dest_ip] = m.group(1)
        if neg:
            data[dest_netmask] = utif.inet_ntoa(~(0xffffffff << (32 - bits)))
        else:
            data[dest_netmask] = utif.inet_ntoa((0xffffffff << (32 - bits)))


def enable_disable_to_boolean_handler(value, data, field):
    if value == 'enable':
        data[field] = True
    if value == 'disable':
        data[field] = False


def date_to_integer_handler(value, data, field):
    if (value == 'now' or value == 'current'):
        data[field] = int(time.time()*1000)

    try:
        data[field] = int(value)
    except:
        pass

    for f,pre in [('%Y-%m-%dT%H:%M:%S', None),
                  ('%Y-%m-%d %H:%M:%S', None),
                  ('%Y-%m-%dT%H:%M:%S%z', None),
                  ('%Y-%m-%d %H:%M:%S%z', None),
                  ('%Y-%m-%d', None),
                  ('%m-%d', '%Y-'),
                  ('%H:%M', '%Y-%m-%dT')]:
        try:
            t = value
            if pre:
                pref = datetime.datetime.now().strftime(pre)
                f = pre + f
                t = pref + t

            thetime = datetime.datetime.strptime(t, f)
            data[field] = int(time.mktime(thetime.timetuple())*1000)
        except:
            pass


HEX_RE = re.compile(r'^0x[0-9a-fA-F]+$')

def hex_to_integer_handler(value, data, field):
    if HEX_RE.match(str(value)):
        _value = str(int(value, 16))
    else:
        _value = str(int(value))
    data[field] = _value


def _invert_netmask(value):
    split_bytes = value.split('.')
    return "%s.%s.%s.%s" % (255-int(split_bytes[0]),
                            255-int(split_bytes[1]),
                            255-int(split_bytes[2]),
                            255-int(split_bytes[3]))


def convert_inverse_netmask_handler(value, data, field):
    data[field] = _invert_netmask(value)


def interface_ranges(names):
    """
    Given a list of interfaces (strings), in any order, with a numeric suffix,
    collect together the prefix components, and create ranges with
    adjacent numeric interfaces, so that a large collection of names
    becomes easier to read.  At the worst, the list will be as
    complicated as the original (which would typically be very unlikely)

    Example: names <- ['Eth0', 'Eth1', 'Eth2',  'Eth4', 'Eth5', 'Eth8']
             result <- ['Eth0-2', 'Eth4-5', 'Eth8']

             names <- ['1','2','3']
             result <- ['1-3']

    """
    # collect the interfaces into dictionaries based on prefixes
    # ahead of groups of digits.
    groups = {}

    def is_digit(c):
        c_ord = ord(c)
        if c_ord >= ord('0') and c_ord <= ord('9'):
            return True
        return False

    for name in names:
        if is_digit(name[-1]):
            for index in range(-2, -len(name)-1, -1):
                if not is_digit(name[index]):
                    index += 1
                    break;
            else:
                index = -len(name)

            prefix = name[:index]
            number = int(name[index:])
            if not prefix in groups:
                groups[prefix] = []
            groups[prefix].append(number)
        else:
            groups[name] = []

    for prefix in groups:
        groups[prefix] = sorted(utif.unique_list_from_list(groups[prefix]))
    
    ranges = []
    for (prefix, value) in groups.items():
        if len(value) == 0:
            ranges.append(prefix)
        else:
            low = value[0]
            prev = low
            for next in value[1:] + [value[-1] + 2]: # +[] flushes last item
                if next > prev + 1:
                    if prev == low:
                        ranges.append('%s%s' % (prefix, low))
                    else:
                        ranges.append('%s%s-%s' % (prefix, low, prev))
                    low = next
                prev = next
 
    return ranges


#print interface_ranges(['1','2','3', 'oe'])
#print interface_ranges(['Eth1','Eth2','Eth3', 'Eth4', 'o5', 'o6'])


def convert_tag_to_parts(value, data, namespace_key, name_key, value_key):
    """
    Split a tag of the form [ns].name=value into the three
    component parts
    """

    if bigsh.description:
        print "convert_tag_to_parts: %s %s %s %s %s" % (
                value, data, namespace_key, name_key, value_key)

    tag_and_value = value.split('=')
    if len(tag_and_value) != 2:
        raise error.ArgumentValidationError("tag <[tag-namespace.]name>=<value>")

    tag_parts = tag_and_value[0].split('.')
    if len(tag_parts) == 1:
        tag_namespace = "default"
        tag_name = tag_parts[0]
    elif len(tag_parts) >= 2:
        tag_namespace = '.'.join(tag_parts[:-1])
        tag_name = tag_parts[-1]

    # should the names have some specific validation?
    data[namespace_key] = tag_namespace
    data[name_key]      = tag_name
    data[value_key]     = tag_and_value[1]


def init_data_handlers(bs):
    global bigsh
    bigsh = bs

    command.add_argument_data_handler('split-cidr-data', split_cidr_data_handler,
                        {'kwargs': {'value': '$value',
                                    'data': '$data',
                                    'dest_ip': '$dest-ip',
                                    'dest_netmask': '$dest-netmask'}})

    command.add_argument_data_handler('split-cidr-data-inverse', split_cidr_data_handler,
                        {'kwargs': {'value': '$value',
                                    'data': '$data',
                                    'dest_ip': '$dest-ip',
                                    'dest_netmask': '$dest-netmask',
                                    'neg' : True}})

    command.add_argument_data_handler('enable-disable-to-boolean', enable_disable_to_boolean_handler,
                        {'kwargs': {'value': '$value',
                                    'data': '$data',
                                    'field': '$field'}})

    command.add_argument_data_handler('date-to-integer', date_to_integer_handler,
                        {'kwargs': {'value' : '$value',
                                    'data'  : '$data',
                                    'field' : '$field'}})

    command.add_argument_data_handler('hex-to-integer', hex_to_integer_handler,
                        {'kwargs': {'value' : '$value',
                                    'data'  : '$data',
                                    'field' : '$field'}})

    command.add_argument_data_handler('convert-inverse-netmask', convert_inverse_netmask_handler,
                        {'kwargs': {'value' : '$value',
                                    'data'  : '$data',
                                    'field' : '$field'}})

    command.add_argument_data_handler('convert-tag-to-parts', convert_tag_to_parts,
                        {'kwargs': {'value'          : '$value',
                                    'data'           : '$data',
                                    'namespace_key'  : '$namespace-key',
                                    'name_key'       : '$name-key',
                                    'value_key'      : '$value-key'}})
