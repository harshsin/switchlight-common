#!/usr/bin/python
############################################################
#
# Generate a new SwitchLight vendor configuration component.
#
import time
import sys
import os
from compgen import ComponentGenerator

class VendorConfigGenerator(ComponentGenerator):
    def __init__(self, vendor):
        self.vendor = vendor
        summary="Vendor Configuration files for %s." % self.vendor
        ComponentGenerator.__init__(self, vendor, "vendor-config-" + self.vendor,
                                    "all", summary, summary)

    def _makefile_dot_comp_all_rules(self):
        return "\t@echo Run 'make deb'"


    def _rules(self):
        return """#!/usr/bin/make -f
VENDOR_NAME=%(vendor)s
include $(SWITCHLIGHT)/make/vendor-config-rules.mk
""" % (self.__dict__)

    def __generate_file(self, path, name, contents):
        with open("%s/%s" % (path, name), "w") as f:
            f.write(contents)

    def _install(self):
        return "/usr/lib/python2.7/dist-packages/*"

    def generate(self, path):
        # Generate the entire component:
        ComponentGenerator.generate(self, path)
        self.path = "%s/%s" % (path, self.vendor)
        # the platform directory layout is this
        os.makedirs('%(path)s/src/python/%(vendor)s' % (self.__dict__))
        self.__generate_file("%(path)s/src/python/%(vendor)s" % (self.__dict__),
                             "__init__.py",
                             "# Vendor classes here")



if __name__ == '__main__':
    if len(sys.argv) != 2:
        print "usage: %s <vendor-name>" % sys.argv[0]
        sys.exit(1)

    vc = VendorConfigGenerator(sys.argv[1])
    vc.generate('.')



