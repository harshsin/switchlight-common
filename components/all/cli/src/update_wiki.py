#!/usr/bin/python

from xmlrpclib import Server
import sys

SERVER      = "https://bigswitch.atlassian.net/wiki/rpc/xmlrpc"
SPACEKEY    = "BSC"
PAGE        = "Switch Light Auto-gen CLI Ref"

BIGWIKI     = "bigwiki.file"

# Use bigswitch account login
# For username, don't include "@bigswitch.com"
# FIXME: take username and password as command line arguments
USERNAME    = ""
PASSWORD    = ""

def main():
    if not USERNAME or not PASSWORD:
        raise Exception("Please set USERNAME and PASSWORD in the script.")

    s = Server(SERVER)
    token = None
    print "XMLPRC Server: %s" % SERVER
    print "Logging in as: %s" % USERNAME
    try:
           token = s.confluence2.login(USERNAME, PASSWORD)
    except Exception as e:
        print "Error logging in. Exception:\n%s" % e
        sys.exit(1)
    print "Login successful. Token: %s" % token

    page = None
    print "Looking for..."
    print "SpaceKey : %s" % SPACEKEY
    print "Page     : %s" % PAGE
    try:
        page = s.confluence2.getPage(token, SPACEKEY, PAGE)
    except Exception as e:
        print "Error finding page. Exception:\n%s" % e
        sys.exit(1)
    print "Page found."

    content = ""
    print "Loading CLI bigwiki file..."
    with open(BIGWIKI) as f:
        content = f.read()

    print "Updating page..."
    try:
        page["content"] = s.confluence2.convertWikiToStorageFormat(token, content)
        s.confluence2.storePage(token, page)
    except Exception as e:
        print "Error updating page. Exception:\n%s" % e
        sys.exit(1)
    print "Update successful. Done."

if __name__ == "__main__":
    main()
