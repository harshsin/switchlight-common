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

    @staticmethod
    def cliTransaction(hostname, port, id):
        try:
            path = "%s?id=%s" % (v1_transaction.route, id)
            response = SLAPIObject.get(hostname, port, path)
            SLAPIObject.dataResult(response.read())
        except:
            pass

    @staticmethod
    def cmdTransaction(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("transaction-status");
            p.add_argument("id")
            p.set_defaults(func=v1_transaction.cmdTransaction)
        else:
            v1_transaction.cliTransaction(sub_parser.hostname, sub_parser.port, sub_parser.id)

class v1_transactions_running(SLAPIObject):
    """Show all running transaction ids"""
    route = "/api/v1/transactions/running"
    def GET(self):
        return SLREST.ok(self.route,
                         data=TransactionManagers.get_tids_running())

    @staticmethod
    def cliRunningTransactions(hostname, port):
        try:
            response = SLAPIObject.get(hostname, port, v1_transactions_running.route)
            SLAPIObject.dataResult(response.read())
        except:
            pass

    @staticmethod
    def cmdRunningTransaction(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("running-transactions");
            p.set_defaults(func=v1_transactions_running.cmdRunningTransaction)
        else:
            v1_transactions_running.cliRunningTransactions(sub_parser.hostname, sub_parser.port)

class v1_transactions_finished(SLAPIObject):
    """Show all finished transaction ids"""
    route = "/api/v1/transactions/finished"
    def GET(self):
        return SLREST.ok(self.route,
                         data=TransactionManagers.get_tids_finished())

    @staticmethod
    def cliFinishedTransactions(hostname, port):
        try:
            response = SLAPIObject.get(hostname, port, v1_transactions_finished.route)
            SLAPIObject.dataResult(response.read())
        except:
            pass

    @staticmethod
    def cmdFinishedTransactions(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("finished-transactions")
            p.set_defaults(func=v1_transactions_finished.cmdFinishedTransactions)

        else:
            v1_transactions_finished.cliFinishedTransactions(sub_parser.hostname, sub_parser.port)

class v1_transactions_all(SLAPIObject):
    """Show all transactions"""
    route = "/api/v1/transactions/all"
    def GET(self):
        return SLREST.ok(self.route,
                         data=TransactionManagers.get_tids_all())

    @staticmethod
    def cliAllTransactions(hostname, port):
        try:
            response = SLAPIObject.get(hostname, port, v1_transactions_all.route)
            SLAPIObject.dataResult(response.read())
        except:
            pass

    @staticmethod
    def cmdAllTransactions(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("all-transactions")
            p.set_defaults(func=v1_transactions_all.cmdAllTransactions)
        else:
            v1_transactions_all.cliAllTransactions(sub_parser.hostname, sub_parser.port)

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

    #dummy functions
    @staticmethod
    def cliFileTransactions():
        return

    @staticmethod
    def cmdFileTransactions(sub_parser, register=False):
        return
