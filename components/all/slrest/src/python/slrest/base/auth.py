#!/usr/bin/python
############################################################
#
# Authentication support. 
#
############################################################
# Obviously temporary, just getting the scaffolding up.
AUTH_DICT = {
    'admin' : 'bsn',
}

def checkpassword(realm, username, password):
    return username in AUTH_DICT and AUTH_DICT[username] == password

def clear():
    """Clear existing authentication pairs."""
    AUTH_DICT.clear()


def add(username, password):
    """Add to existing authentication pairs."""
    AUTH_DICT[username] = password




