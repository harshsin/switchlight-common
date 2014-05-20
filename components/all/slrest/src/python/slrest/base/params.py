#!/usr/bin/python
############################################################
#
# Parameter Validation
#
############################################################
import sys
import os
import subprocess
import logging
import inspect
import logging
import threading
import signal

def  boolean(name, param):
    """Validate the the input is a boolean value."""
    if param in ['0', 'False', 'false']:
        return (False, None)
    if param in ['1', 'True', 'true']:
        return (True, None)
    return (None, "The %s parameter (%s) is not a valid boolean." % (name, param))


def integer(name, param):
    """Validate that the input is a valid signed integer."""
    try:
        param = int(param)
        return (param, None)
    except ValueError:
        return (None, "The %s parameter (%s) is not a valid integer." % (name, param))

def uinteger(name, param):
    """Validate that the input is a valid unsigned integer."""
    (param, error) = integer(name, param)
    if error:
        return (param, error)
    if param < 0:
        return (None, "The %s parameter (%s) is not a valid positive integer." % (name, param))
    return (param, error)


