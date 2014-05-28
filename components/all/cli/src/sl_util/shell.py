# Copyright (c) 2013-2014  BigSwitch Networks

# Some portions:
# Copyright (c) 2011-2013  Barnstormer Softworks, Ltd.
# License: GENI Public License

import subprocess
import time
import os
import signal
import error

def call (cmd, show_cmd = False, show_output = False, raise_exc = True):
  if show_cmd:
    print cmd 
  if show_output:
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
    soutl = []
    for line in iter(p.stdout.readline, ""):
      soutl.append(line)
      print line,
    sout = "".join(soutl)
    serr = ""
  else:
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    (sout, serr) = p.communicate()
  if raise_exc:
    if p.returncode != None and p.returncode != 0:
      raise subprocess.CalledProcessError(p.returncode, cmd)
  return (sout, serr, p.returncode)

def call_poll (cmd, timeout = 60, raise_exc = True):

  #adding "exec" to cmd will cause cmd to inherit the shell process,
  #instead of having the shell launch a child process, which does not get killed  
  p = subprocess.Popen("exec " + cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
  t_ = time.time() + timeout
  while p.poll() == None:
    if time.time() > t_:
      p.kill()
      #A negative return value indicates that the child was terminated by SIGKILL  
      raise subprocess.CalledProcessError(-signal.SIGKILL, cmd)
    time.sleep(1)

  (sout, serr) = p.communicate()
  if raise_exc:
    if p.returncode != None and p.returncode != 0:
      raise subprocess.CalledProcessError(p.returncode, cmd)
  return (sout, serr, p.returncode)
