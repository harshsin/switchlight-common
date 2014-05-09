#!/usr/bin/python
############################################################
#
# SLREST Response Constants
#
############################################################

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



