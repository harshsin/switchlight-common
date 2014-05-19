#!/usr/bin/python
############################################################
#
# Transaction Manager APIs
#
############################################################
import cherrypy
import logging
import json

from slrest.base.slapi_object import SLAPIObject
from slrest.base import util
from slrest.base.transact import *
from slrest.base.response import SLREST


class v1_transaction(SLAPIObject):
    """Get the status of the given transaction."""
    route = "/api/v1/transaction"
    def GET(self, id):
        tt = TransactionManagers.get_task(id)
        if tt:
            return tt.response()
        else:
            return SLREST.response(self.route, SLREST.Status.MISSING)

class v1_transactions_running(SLAPIObject):
    """Show all running transaction ids"""
    route = "/api/v1/transactions/running"
    def GET(self):
        return SLREST.response(self.route, SLREST.Status.OK,
                               data=TransactionManagers.get_tids_running())

class v1_transactions_finished(SLAPIObject):
    """Show all finished transaction ids"""
    route = "/api/v1/transactions/finished"
    def GET(self):
        return SLREST.response(self.route, SLREST.Status.OK,
                               data=TransactionManagers.get_tids_finished())

class v1_transactions_all(SLAPIObject):
    """Show all transactions"""
    route = "/api/v1/transactions/all"
    def GET(self):
        return SLREST.response(self.route, SLREST.Status.OK,
                               data=TransactionManagers.get_tids_all())



