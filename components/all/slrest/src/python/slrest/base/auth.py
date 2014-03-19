#!/usr/bin/python
############################################################
#
# User and Network Authentication support. 
#
############################################################
import crypt
import spwd
from netaddr import *

#
# Optional Server account credentials. 
#
SERVER_ACCOUNTS = { 
}

def server_account_clear_all():
    """Clear existing authentication pairs."""
    SERVER_ACCOUNTS.clear()

def server_account_get_all():
    return SERVER_ACCOUNTS

def server_account_add(username, password):
    """Add to existing authentication pairs."""
    SERVER_ACCOUNTS[username] = password

def server_account_update(d):
    SERVER_ACCOUNTS.update(d)

def server_account_remove(username):
    SERVER_ACCOUNTS.pop(username, None)
 
def server_account_check(username, password):
    """Check a user against the local authorization dict."""
    if username not in SERVER_ACCOUNTS:
        return None
    return SERVER_ACCOUNTS[username] == password



#
# Local account authorizations
#
# Only user accounts in the whitelist will be checked:
USER_ACCOUNTS = {
}

def user_account_clear_all():
    USER_ACCOUNTS.clear()
def user_account_get_all():
    return USER_ACCOUNTS

def user_account_update(d):
    USER_ACCOUNTS.update(d)

def user_account_add(username):
    USER_ACCOUNTS[username] = True

def user_account_remove(username):
    USER_ACCOUNTS.pop(username, None)

def user_account_check(username, password):

    if (username not in USER_ACCOUNTS or USER_ACCOUNTS[username] is False) and ('*' not in USER_ACCOUNTS):
        # User is not enabled
        return None

    try:
        hashedpw = spwd.getspnam(username)[1]
    except KeyError, e:
        # User does not exist
        return None

    if hashedpw in ["NP", "!", "", "LK", "*", "!!", None]:
        # No password, account locked, or password expired. Do not allow. 
        return False
    
    return crypt.crypt(password, hashedpw) == hashedpw


#
# Main user authorizations routine. This is passed as the validator
# to cherrypy. 
#
def checkpassword(realm, username, password):
    if server_account_check(username, password):
        return True
    if user_account_check(username, password):
        return True

    return False


#
# Allowed networks
#
# Only requests from IPs in the allowed networks will be allowed.
#
NETWORKS = {
}

def network_clear_all():
    NETWORKS.clear()

def network_get_all():
    return NETWORKS

def network_add(key, network):
    NETWORKS[key] = network

def network_remove(key):
    NETWORKS.pop(key, None)

def network_update(d):
    NETWORKS.update(d)

def network_check(address):
    for (key, data) in NETWORKS.iteritems():
        if IPAddress(address) in IPNetwork(data):
            return data

    return False


