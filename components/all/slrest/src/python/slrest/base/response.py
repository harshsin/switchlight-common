#!/usr/bin/python
############################################################
#
# SLREST Responses
#
############################################################
import json

class SLREST(object):
    class Keys(object):
        """
        SLREST JSON Standard Response Keys
        """
        PATH="path"
        STATUS="status"
        REASON="reason"
        TRANSACTION="transaction"
        DATA="data"

    class Status(object):
        """
        SLREST Status Codes

        All APIs set the status key of their response to one
        of these values.
        """
        OK="OK"
        ERROR="ERROR"
        ACCEPTED="ACCEPTED"
        BUSY="BUSY"
        PENDING="PENDING"
        EXPIRED="EXPIRED"
        MISSING="MISSING"

    @staticmethod
    def response(path, status, reason, transaction, data):
        r = {}
        r[SLREST.Keys.PATH] = path
        r[SLREST.Keys.STATUS] = status
        r[SLREST.Keys.REASON] = reason
        r[SLREST.Keys.TRANSACTION] = transaction
        r[SLREST.Keys.DATA] = data
        return json.dumps(r)
