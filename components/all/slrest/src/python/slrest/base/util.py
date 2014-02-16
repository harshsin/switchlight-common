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
    p = subprocess.Popen(('/usr/bin/pcli', '--init', '--mode=config'), stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.STDOUT)
    (out,err) = p.communicate(input=cmd)
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
