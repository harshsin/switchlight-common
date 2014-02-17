#!/usr/bin/python
############################################################
#
# Authentication support. 
#
############################################################
import crypt
import spwd

logger = None

#
# Optional Server Authorization Credentials. 
#
SERVER_AUTHORIZATIONS = {
    'admin' : 'bsn',
}

def server_auth_clear_all():
    """Clear existing authentication pairs."""
    SERVER_AUTHORIZATIONS.clear()

def server_auth_add(username, password):
    """Add to existing authentication pairs."""
    SERVER_AUTHORIZATIONS[username] = password

def server_auth_remove(username):
    SERVER_AUTHORIZATIONS.pop(username, None)
    

def server_auth_check(username, password):
    """Check a user against the local authorization dict."""
    if username not in SERVER_AUTHORIZATIONS:
        return None
    return SERVER_AUTHORIZATIONS[username] == password



#
# Local account authorizations
#
# Only user accounts in the whitelist will be checked:
USER_ACCOUNTS = {
    'admin' : True, 
    'root' : True,
}

def user_account_clear_all():
    USER_ACCOUNTS.clear()

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
# Main authorizations routine. This is passed as the validator
# to cherrypy. 
#
def checkpassword(realm, username, password):
    if server_auth_check(username, password):
        return True
    if user_account_check(username, password):
        return True

    return False




