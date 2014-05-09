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

        @classmethod
        def valid(klass, s):
            # Close enough
            return s in klass.__dict__

    @staticmethod
    def response(path, status, reason=None, transaction=None, data=None):
        r = {}

        if reason is None:
            reason = status

        r[SLREST.Keys.PATH] = path
        r[SLREST.Keys.STATUS] = status
        r[SLREST.Keys.REASON] = reason
        r[SLREST.Keys.TRANSACTION] = transaction
        r[SLREST.Keys.DATA] = data

        #
        # Enforce protocol requirements.
        # We don't throw an exception here to communicate it
        # but hijack the message. This is mostly for developer
        # testing.
        #
        ereasons = ''
        if path is None:
            ereasons += "The PATH key is None. This should be fixed."
        if not SLREST.Status.valid(status):
            ereasons += "The reported status value '%s' is invalid." % status

        if len(ereasons):
            # Return as an error messsage with the original content in data field:
            e = {}
            e[SLREST.Keys.STATUS] = SLREST.Status.ERROR
            e[SLREST.Keys.REASON] = "SLREST Protocol Error. Reasons:" + ereasons + "The data key contains the original response."
            e[SLREST.Keys.DATA] = r
            r = e

        return json.dumps(r)
