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
            return SLREST.missing(self.route)

class v1_transactions_running(SLAPIObject):
    """Show all running transaction ids"""
    route = "/api/v1/transactions/running"
    def GET(self):
        return SLREST.ok(self.route,
                         data=TransactionManagers.get_tids_running())

class v1_transactions_finished(SLAPIObject):
    """Show all finished transaction ids"""
    route = "/api/v1/transactions/finished"
    def GET(self):
        return SLREST.ok(self.route,
                         data=TransactionManagers.get_tids_finished())

class v1_transactions_all(SLAPIObject):
    """Show all transactions"""
    route = "/api/v1/transactions/all"
    def GET(self):
        return SLREST.ok(self.route,
                         data=TransactionManagers.get_tids_all())


class v1_transaction_file(SLAPIObject):
    """Retrieve a transaction file."""
    route = "/api/v1/transactions/file"
    def GET(self, id, fid):
        tt = TransactionManagers.get_task(id)
        if tt:
            try:
                (filename, content_type) = tt.files[fid]
                result = cherrypy.lib.static.serve_file(
                    filename, content_type=content_type,
                    disposition='attachment',
                    name=os.path.basename(filename))
                return result
            except (AttributeError, KeyError), e:
                return SLREST.missing(self.route, "The file is not recognized for this transaction.")
        else:
            return SLREST.missing(self.route, "The transaction id does not exist.")

