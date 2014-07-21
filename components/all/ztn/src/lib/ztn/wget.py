#!/usr/bin/python

"""wget.py

Dumb replacement for 'wget -O' that handles IPv6 link local.
"""

import urllib, urlparse, httplib
import re
import socket
import sys

dst = sys.argv[1]
url = sys.argv[2]

# match any link local IPv6 address spec
LL_RE = re.compile("([fF][eE]80:[0-9a-fA-F]*:[0-9a-fA-F]*:[0-9a-fA-F]*:[0-9a-fA-F]*:[0-9a-fA-F]*)%(.*)")

# match any link local IPv6 http host spec (with brackets and maybe port)
LL2_RE = re.compile("[[]([fF][eE]80:[0-9a-fA-F]*:[0-9a-fA-F]*:[0-9a-fA-F]*:[0-9a-fA-F]*:[0-9a-fA-F]*)%(.*)[]](:[0-9][0-9]+)?")

class MyURLopener(urllib.FancyURLopener):

    def open_http6(self, url, data=None):
        """Handle GET and POST with IPv6 link local.

        Do not pass in the IPv6 link local host as 'Host',
        the server will reject the request.

        copy-pasta here from urllib.open_http,
        except for the above changes.
        """

        host, selector = urllib.splithost(url)

        h = httplib.HTTP(host)
        if data is not None:
            h.putrequest('POST', selector)
            h.putheader('Content-Type', 'application/x-www-form-urlencoded')
            h.putheader('Content-Length', '%d' % len(data))
        else:
            h.putrequest('GET', selector)
        for args in self.addheaders: h.putheader(*args)
        h.endheaders(data)

        errcode, errmsg, headers = h.getreply()
        fp = h.getfile()
        if errcode == -1:
            if fp: fp.close()
            # something went wrong with the HTTP status line
            raise IOError, ('http protocol error', 0,
                            'got a bad status line', None)
        # According to RFC 2616, "2xx" code indicates that the client's
        # request was successfully received, understood, and accepted.
        if (200 <= errcode < 300):
            return urllib.addinfourl(fp, headers, "http:" + url, errcode)
        else:
            if data is None:
                return self.http_error(url, fp, errcode, errmsg, headers)
            else:
                return self.http_error(url, fp, errcode, errmsg, headers, data)

    def open_http(self, url, data=None):

        host, selector = urllib.splithost(url)

        # proper link local syntax
        m = LL2_RE.match(host)
        if m is not None:
            return self.open_http6(url)

        # also match the hacked version of link local addresses
        # supported by busybox wget
        # (this version does not support alternate ports)
        m = LL_RE.match(host)
        if m is not None:
            rec = list(urlparse.urlparse(url))
            rec[1] = "[" + rec[1] + "]"
            return self.open_http6(urlparse.urlunparse(rec))

        return urllib.FancyURLopener.open_http(self, url, data=data)


class MyHTTPConnection(httplib.HTTPConnection):

    def connect6(self):

        addrinfo = socket.getaddrinfo(self.host, self.port,
                                      socket.AF_INET6, socket.SOCK_STREAM)
        (family, socktype, proto, canonname, sockaddr) = addrinfo[0]
        self.sock = socket.socket(family, socktype, proto)
        if self.source_address:
            self.sock.bind(self.source_address)
        if self.timeout is not socket._GLOBAL_DEFAULT_TIMEOUT:
            self.sock.settimeout(self.timeout)
        self.sock.connect(sockaddr)

    def connect(self):

        m = LL_RE.match(self.host)
        if m is not None:
            self.connect6()
        else:
            httplib.HTTPConnection.connect(self)

def reporthook(blocknum, blocksize, totalsize):
    sys.stderr.write('.')

httplib.HTTP._connection_class = MyHTTPConnection

urllib._urlopener = MyURLopener()

sys.stderr.write("downloading %s --> %s\n"
                 % (url, dst,))
urllib.urlretrieve(url, dst, reporthook=reporthook)
sys.exit(0)
