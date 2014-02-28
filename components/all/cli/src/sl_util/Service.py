# Copyright (c) 2013  BigSwitch Networks

from __future__ import absolute_import

from sl_util.shell import call

deferred_restart = []

class Service(object):
    SVC_NAME = "UNSET"
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


    @staticmethod
    def handle_deferred_restart ():
        for svc in deferred_restart:
            print "Restarting %s..." % svc.SVC_NAME
            svc.restart()
