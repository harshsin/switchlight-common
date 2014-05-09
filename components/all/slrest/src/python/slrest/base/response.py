#!/usr/bin/python
############################################################
#
# SLREST Response Formats
#
############################################################

class status(object):
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

