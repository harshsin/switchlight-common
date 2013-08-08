# Copyright (c) 2013  BigSwitch Networks

# Some portions:
# Copyright (c) 2011-2013  Barnstormer Softworks, Ltd.
# License: GENI Public License

import subprocess

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
