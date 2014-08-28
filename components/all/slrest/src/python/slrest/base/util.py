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

def check_output(cmd, *args, **kwargs):

    kwargs = dict(kwargs)

    stderr = kwargs.pop('stderr', None)
    if isinstance(stderr, logging.Logger):
        kwargs['stderr'] = subprocess.PIPE
    kwargs['stdout'] = subprocess.PIPE

    pipe = subprocess.Popen(cmd, *args, **kwargs)
    out, err = pipe.communicate()

    if err and isinstance(stderr, logging.Logger):
        for line in err.strip().splitlines():
            stderr.warn("check_output: >>> %s", line)

    rc = pipe.wait()
    if not rc: return out

    raise subprocess.CalledProcesssError(rc, cmd, output=out)

def pcli_command(cmd, stderr=subprocess.STDOUT):
    """Execute a PCLI command and return the results."""
    cmd = ('/usr/bin/pcli', '--init', '--mode=config', '--command', cmd)
    return check_output(cmd, stderr=stderr)

def bash_command(cmd, stderr=subprocess.STDOUT):
    """Execute a bash command and return the results.

    Set 'stderr' as per standard subprocess.py,
    or set to a logger object to send to stderr at WARN level.
    """
    try:
        out = check_output(cmd, shell=True, stderr=stderr)
        rc = 0
    except subprocess.CalledProcessError, e:
        out = e.output
        rc = e.returncode
    return rc, out

def reboot(logger, seconds=None):
    """Reboot after the given number of seconds."""
    if seconds:
        threading.Timer(seconds, lambda: reboot(logger)).start()
    else:
        if logger:
            logger.info("Rebooting...")
        os.kill(1, signal.SIGINT)
