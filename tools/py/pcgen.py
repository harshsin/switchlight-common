#!/usr/bin/python
############################################################
#
# Generate a new SwitchLight platform configuration component.
#
import time
import sys
import os
from compgen import ComponentGenerator

class PlatformConfigGenerator(ComponentGenerator):
    def __init__(self, platform, vendor):
        self.platform = platform
        self.vendor = vendor
        self.Vendor = vendor.title()
        summary="Platform Configuration files for %s." % self.platform
        ComponentGenerator.__init__(self, platform, "platform-config-" + self.platform,
                                    "all", summary, summary)

    def _required_packages(self):
        p = "vendor-config-switchlight:all"
        if self.vendor != "none":
            p += " vendor-config-%s:all" % self.vendor
        return p

    def _makefile_dot_comp_all_rules(self):
        return "\t@echo Run 'make deb'"


    def _rules(self):
        return """#!/usr/bin/make -f
PLATFORM_NAME=%(platform)s
BRCM_PLATFORM_NAME=none
include $(SWITCHLIGHT)/make/platform-config-rules.mk
""" % (self.__dict__)

    def __generate_file(self, path, name, contents):
        if not os.path.exists(path):
            os.makedirs(path)
        with open("%s/%s" % (path, name), "w") as f:
            f.write(contents)

    def _install(self):
        return "/lib/platform-config/*"

    def generate(self, path):
        # Generate the entire component:
        ComponentGenerator.generate(self, path)
        self.path = "%s/%s" % (path, self.platform)
        # the platform directory layout is this
        os.makedirs('%(path)s/src/install' % (self.__dict__))
        os.makedirs('%(path)s/src/boot' % (self.__dict__))
        os.makedirs('%(path)s/src/sbin' % (self.__dict__))
        os.makedirs('%(path)s/src/python' % (self.__dict__))
        self.__generate_file('%(path)s/src' % self.__dict__, 'name', self.platform+'\n')
        self.__generate_file('%(path)s/src/install' % self.__dict__,
                             '%(platform)s.sh' % self.__dict__,
                             "# Platform data goes here.")
        self.__generate_file('%(path)s/src/sbin' % self.__dict__,
                             'gpio_init',
                             """# platform: %(platform)s
exit 0
""" % self.__dict__)
        self.__generate_file('%(path)s/src/boot' % self.__dict__,
                             self.platform,
                             "# Platform data goes here.")
        self.__generate_file('%(path)s/src/boot' % self.__dict__,
                             'detect.sh',
                             """# Default platform detection.
if grep -q "^model.*: %(platform)s$" /proc/cpuinfo; then
    echo "%(platform)s" >/etc/sl_platform
    exit 0
else
    exit 1
fi

""" % self.__dict__)
        self.__generate_file('%(path)s/src/python' % self.__dict__,
                             'slpc.py',
                             """#!/usr/bin/python
############################################################
#
# Platform Driver for %(platform)s
#
############################################################
import os
import struct
import time
import subprocess
from switchlight.platform.base import *
from switchlight.vendor.quanta import *

class SwitchLightPlatformImplementation(SwitchLightPlatform%(Vendor)s):

    def model(self):
        raise Exception()

    def platform(self):
        return '%(platform)s'

    def _plat_info_dict(self):
        raise Exception()

    def oid_table(self):
        raise Exception()


if __name__ == "__main__":
    print SwitchLightPlatformImplementation()

""" % self.__dict__)


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print "usage: %s <platform-name> <vendor-name>" % sys.argv[0]
        sys.exit(1)
    pc = PlatformConfigGenerator(sys.argv[1], sys.argv[2])
    pc.generate('.')
