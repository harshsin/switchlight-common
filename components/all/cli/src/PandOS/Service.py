# Copyright (c) 2013  BigSwitch Networks

from __future__ import absolute_import

from PandOS.shell import call

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
    def restart (klass):
        (sout, serr, ret) = call("service %s restart" % (klass.SVC_NAME))

    @classmethod
    def reload (klass):
        (sout, serr, ret) = call("service %s reload" % (klass.SVC_NAME))

