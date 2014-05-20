#!/usr/bin/python
############################################################
#
# Common framework for all REST api implementations.
#
############################################################
import cherrypy


############################################################
#
# All implementation classes are derived from slapi_object
#
# Derived classes must provide the following:
#
# 1. Be derived directly from slapi_object
# 2. Contain a class variable named 'route' with the REST api mount point.
# 3. Either a GET, POST, and/or PUT method.
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
