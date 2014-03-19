#!/usr/bin/python
############################################################
# <bsn.cl fy=2013 v=none>
#
#        Copyright 2013, 2014 BigSwitch Networks, Inc.
#
#
#
# </bsn.cl>
############################################################
#
# Sensor processing support. 
#
# Most of this would normally be handled by the sensors3.conf
# file. 
#
# We need additional control and semantic processing of the 
# output of the sensors so process the raw output directly. 
#
############################################################
import subprocess
import re

def sensors(args=None):
    """Run the sensors binary and return the output."""
    argv= ['/usr/bin/sensors']
    if args:
        if type(args) is str:
            argv.append(args)
        else:
            argv = argv + args
    p = subprocess.Popen(argv, stdout=subprocess.PIPE,
                         stdin=subprocess.PIPE, stderr=subprocess.PIPE)
    return p.communicate()


def sensor_dict(sensor, mapdict=None, ignore=True, filter_=None):
    """Returns a dict of values for the given sensor.

    If mapdict is provided it is queried to map sensor names. 
    If ignore is True, then sensor fields not in the mapdict will be ignored.
    filter_ allows optional filtering or transformation of the data before it is returned. 
    """
    (o, e) = sensors(sensor)
    data = {}
    
    for l in o.split('\n')[2:]:
        l = l.replace('ALARM', '')
        m = re.match('(?P<field>.*?):\s+(?P<value>.*)', l)
        if m:
            f = m.group('field')
            if mapdict and f not in mapdict and ignore:
                continue
            if mapdict and f in mapdict:
                f = mapdict[f]
            info = {}
            info['all'] = m.group('value')
            info['field'] = m.group('field')
            mm = re.match('(?P<main>.*?)\((?P<limits>.*?\))', m.group('value'))
            if mm:
                info['value'] = mm.group('main').strip()
                info['limits'] = mm.group('limits').strip()
            else:
                info['value'] = info['all'].strip()
                    
            if filter_:
                info = filter_(f, info)

            if info:
                data[f] = info

    return data

    
def sensor_values(sensor, mapdict=None, ignore=True, filter_=None, indent=''):
    """Returns a string of indicated sensors values."""
    data = sensor_dict(sensor, mapdict, ignore, filter_)
    s = ''
    for k in sorted(data):
        s += "%s%s:     %s\n" % (indent, k, data[k]['value'])
    return s

def sensor_scale(value, divisor):
    m = re.match('(?P<prefix>.*?)(?P<v>\d+\.\d\d)(?P<suffix>.*)', value)
    if m:
        v = float(m.group('v'))
        v = v / divisor
        return "%s%.2f%s" % (m.group('prefix'), v, m.group('suffix'))

    m = re.match('(?P<prefix>.*?)(?P<v>\d+)(?P<suffix>.*)', value)
    if m:
        v = int(m.group('v'))
        v = v / divisor
        return "%s%d%s" % (m.group('prefix'), v, m.group('suffix'))

    return value

