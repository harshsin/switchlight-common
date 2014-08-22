# Copyright (c) 2013-2014  BigSwitch Networks

# Some portions:
# Copyright (c) 2011-2013  Barnstormer Softworks, Ltd.
# License: GENI Public License

import subprocess
import time
import os
import signal

def call (cmd, show_cmd = False, show_output = False, raise_exc = True, timeout = None):
  if show_cmd:
    print cmd 
  #adding "exec" to cmd will cause cmd to inherit the shell process,
  #instead of having the shell launch a child process, which does not get killed
  p = subprocess.Popen("exec " + cmd if timeout is not None else cmd,
                       stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT if show_output else subprocess.PIPE,
                       shell=True)
  if timeout is not None:
    t_ = time.time() + timeout
    while p.poll() is None:
      if time.time() > t_:
        p.kill()
        #A negative return value indicates that the child was terminated by SIGKILL
        if raise_exc:
          raise subprocess.CalledProcessError(-signal.SIGKILL, cmd)
        else:
          return (None, None, -signal.SIGKILL)
      time.sleep(1)
  if show_output:
    soutl = []
    for line in iter(p.stdout.readline, ""):
      soutl.append(line)
      print line,
    sout = "".join(soutl)
    serr = ""
    p.wait()
  else:
    (sout, serr) = p.communicate()
  if raise_exc:
    if p.returncode != None and p.returncode != 0:
      raise subprocess.CalledProcessError(p.returncode, cmd)
  return (sout, serr, p.returncode)
