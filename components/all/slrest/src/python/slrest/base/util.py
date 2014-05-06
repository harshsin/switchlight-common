#!/usr/bin/python
############################################################
#
# Utilities
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


def pcli_command(cmd):
    """Execute a PCLI command and return the results."""
    out = subprocess.check_output(('/usr/bin/pcli', '--init', '--mode=config',
                                   '--command', cmd), 
                                  stderr=subprocess.STDOUT)
    return out


def bash_command(cmd):
    """Execute a bash command and return the results."""
    try:
        result = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError, e:
        result = e.output
    return result


def reboot(logger, seconds=None):
    """Reboot after the given number of seconds."""
    if seconds:
        threading.Timer(seconds, lambda: reboot(logger)).start()
    else:
        if logger:
            logger.info("Rebooting...")
        os.kill(1, signal.SIGINT)
