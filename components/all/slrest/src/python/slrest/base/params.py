#!/usr/bin/python
############################################################
#
# Parameter Validation
#
############################################################

def  boolean(name, param):
    if param in ['0', 'False', 'false']:
        return (False, None)
    if param in ['1', 'True', 'true']:
        return (True, None)
    return (None, "The %s parameter (%s) is not a valid boolean." % (name, param))


def integer(name, param):
    try:
        param = int(param)
        return (param, None)
    except ValueError:
        return (None, "The %s parameter (%s) is not a valid integer." % (name, param))



