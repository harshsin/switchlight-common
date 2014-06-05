#!/usr/bin/python
############################################################
#
# Common framework for all REST api implementations.
#
############################################################
import cherrypy
import urllib
import urllib2
import httplib
import traceback
import json
from pprint import pprint

############################################################
#
# All implementation classes are derived from slapi_object
#
# Derived classes must provide the following:
#
# 1. Be derived directly from slapi_object
# 2. Contain a class variable named 'route' with the REST api mount point.
# 3. Either a GET, POST, and/or PUT method.
# 4. Contain a method starting with cmd to register the handler for slrest client.
# 5. Contain a method starting with cli to handle slrest client request.
#
############################################################

class SLAPIObject(object):
    """Common base class for all REST implementation classes."""
    exposed=True
    route=None
    mounted = []

    def __init__(self, logger):
        self.logger = logger

    @staticmethod
    def mount_all(logger):
        """Mount all subclasses"""
        for klass in SLAPIObject.__subclasses__():
            if klass.route is None:
                raise ValueError("There is no route specified for the class %s" % klass)

            logger.info("Mounting class %s @ %s" % (klass.__name__, klass.route))
            cherrypy.tree.mount(
                klass(logger), klass.route,
                {'/':
                     {
                        'request.dispatch': cherrypy.dispatch.MethodDispatcher(),
                        }
                 }
                )
            SLAPIObject.mounted.append(klass)

    @staticmethod
    def mount_cmds(sub_parser):
        """Mount all slrest cli handlers"""
        for klass in SLAPIObject.mounted:
            for f in dir(klass):
                if f.startswith("cmd"):
                    getattr(klass, f)(sub_parser, True)

    @staticmethod
    def connect(url):
        username = 'root'
        password = 'bsn'

        p = urllib2.HTTPPasswordMgrWithDefaultRealm()
        p.add_password(None, url, username, password)

        handler = urllib2.HTTPBasicAuthHandler(p)
        opener = urllib2.build_opener(handler)
        urllib2.install_opener(opener)

    @staticmethod
    def get(hostname, port, route):
        url = 'https://%s:%s/%s' % (hostname, port, route)
        SLAPIObject.connect(url)
        try:
            response = urllib2.urlopen(url)
        except urllib2.HTTPError, e:
            print 'HTTPError = ' + str(e.code)
            raise
        except urllib2.URLError, e:
            print 'URLError = ' + str(e.reason)
            raise
        except httplib.HTTPException, e:
            print 'HTTPException'
            raise
        except Exception:
           print 'generic exception: ' + traceback.format_exc()
           raise

        return response

    @staticmethod
    def post(hostname, port, route, query_args):
        url = 'https://%s:%s/%s' % (hostname, port, route)
        SLAPIObject.connect(url)
        encoded_args = urllib.urlencode(query_args)
        try:
            response = urllib2.urlopen(url, encoded_args)
        except urllib2.HTTPError, e:
            print 'HTTPError = ' + str(e.code)
            raise
        except urllib2.URLError, e:
            print 'URLError = ' + str(e.reason)
            raise
        except httplib.HTTPException, e:
            print 'HTTPException'
            raise
        except Exception:
           print 'generic exception: ' + traceback.format_exc()
           raise

        return response

    @staticmethod
    def fix_unicode(data):
        if isinstance(data, unicode):
            return data.encode('utf-8')
        elif isinstance(data, dict):
            data = dict((SLAPIObject.fix_unicode(k), SLAPIObject.fix_unicode(data[k])) for k in data)
        elif isinstance(data, list):
            for i in xrange(0, len(data)):
                data[i] = SLAPIObject.fix_unicode(data[i])
        return data

    @staticmethod
    def printJsonData(jdata):
        data = SLAPIObject.fix_unicode(json.loads(jdata))
        for key, value in data.items():
            print '%s: %s' % (key, value)

    @staticmethod
    def dataResult(page, json_output=False):
        rv = json.loads(page)
        if rv['status'] == 'OK':
            if json_output:
                SLAPIObject.printJsonData(rv['data'])
            elif rv['data'] is not None:
                print SLAPIObject.fix_unicode(rv['data'])
            else:
                print rv['reason']
        elif rv['status'] == 'ERROR':
            print rv['reason']
        else:
            pprint(SLAPIObject.fix_unicode(rv))
        return rv

class help(SLAPIObject):
    """Show available API commands and their descriptions."""
    route="/api/help"
    def GET(self):
        rv = "Available REST APIs:\n"
        routes = {}
        # Sort output by route
        for klass in sorted(SLAPIObject.mounted, key=lambda k: k.route):
            rv += "    %-32s %s\n" % (klass.route, klass.__doc__)
        return rv

    @staticmethod
    def cliHelp(hostname, port):
        try:
            response = SLAPIObject.get(hostname, port, help.route)
            print response.read()
        except:
            pass

    @staticmethod
    def cmdHelp(sub_parser, register=False):
        if register:
            p = sub_parser.add_parser("help")
            p.set_defaults(func=help.cmdHelp)
        else:
            help.cliHelp(sub_parser.hostname, sub_parser.port)
