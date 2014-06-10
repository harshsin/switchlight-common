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
import locale


def pcli_command(cmd):
    """Execute a PCLI command and return the results."""
    locale.setlocale(locale.LC_ALL, '')
    out = subprocess.check_output(('/usr/bin/pcli', '--init', '--mode=config',
                                   '--command', cmd),
                                  stderr=subprocess.STDOUT)
    return out


def bash_command(cmd):
    """Execute a bash command and return the results."""
    rc = 0
    try:
        output = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError, e:
        output = e.output
        rc = e.returncode

    return (rc, output)

def reboot(logger, seconds=None):
    """Reboot after the given number of seconds."""
    if seconds:
        threading.Timer(seconds, lambda: reboot(logger)).start()
    else:
        if logger:
            logger.info("Rebooting...")
        os.kill(1, signal.SIGINT)
