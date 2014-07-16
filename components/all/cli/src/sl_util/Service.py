# Copyright (c) 2013  BigSwitch Networks

from __future__ import absolute_import

from sl_util.shell import call
from sl_util import const

import os
import shutil

deferred_restart = []

UNSET = "UNSET"

class Service(object):
    SVC_NAME = UNSET
    CFG_PATH = UNSET
    RUNNING = True
    HALTED = False

    @classmethod
    def disable (klass):
        (sout, serr, ret) = call("service %s stop" % (klass.SVC_NAME))

    @classmethod
    def enable (klass):
        (sout, serr, ret) = call("service %s start" % (klass.SVC_NAME))
     
    @classmethod
    def status (klass):
        (sout, serr, ret) = call("service %s status" % (klass.SVC_NAME), raise_exc=False)
        if ret == 0:
            return Service.RUNNING
        else:
            return Service.HALTED

    @classmethod
    def restart (klass, deferred=False):
        if deferred:
            print "Warning: %s restart is deferred." % klass.SVC_NAME
            global deferred_restart
            if klass not in deferred_restart:
                deferred_restart.append(klass)
        else:
            (sout, serr, ret) = call("service %s restart" % (klass.SVC_NAME))

    @classmethod
    def reload (klass):
        (sout, serr, ret) = call("service %s reload" % (klass.SVC_NAME))

    @classmethod
    def save_default_settings (klass):
        ws = os.path.join(const.DEFAULT_DIR, klass.SVC_NAME)
        if os.path.exists(ws):
            print "Default settings for %s already exist." % klass.SVC_NAME
            return

        conf_dir = os.path.join(ws, "conf")
        state_dir = os.path.join(ws, "state")
        os.makedirs(conf_dir)
        os.makedirs(state_dir)

        # save config
        if klass.CFG_PATH != UNSET:
            dst = os.path.join(conf_dir, os.path.basename(klass.CFG_PATH))
            if os.path.exists(klass.CFG_PATH):
                shutil.copy(klass.CFG_PATH, dst)
        else:
            print "CFG_PATH of %s is not set." % klass.SVC_NAME

        # save state
        running = os.path.join(state_dir, "running")
        if klass.status() == Service.RUNNING:
            call("touch %s" % running)

    @classmethod
    def revert_default_settings (klass):
        ws = os.path.join(const.DEFAULT_DIR, klass.SVC_NAME)
        if not os.path.exists(ws):
            print "Default settings for %s do not exist." % klass.SVC_NAME
            return

        conf_dir = os.path.join(ws, "conf")
        state_dir = os.path.join(ws, "state")

        # revert config
        if klass.CFG_PATH != UNSET:
            src = os.path.join(conf_dir, os.path.basename(klass.CFG_PATH))
            if os.path.exists(src):
                shutil.copy(src, klass.CFG_PATH)
            elif os.path.exists(klass.CFG_PATH):
                os.unlink(klass.CFG_PATH)

        # revert state
        running = os.path.join(state_dir, "running")
        if os.path.exists(running):
            klass.restart()
        else:
            klass.disable()

    @staticmethod
    def handle_deferred_restart ():
        for svc in deferred_restart:
            print "Restarting %s..." % svc.SVC_NAME
            svc.restart()
