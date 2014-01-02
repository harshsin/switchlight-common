#!/usr/bin/python
############################################################
#
# Generate a new SwitchLight component directory.
#

import time
import sys
import os
from debgen import DebianGenerator

class ComponentGenerator(DebianGenerator):
    def __init__(self, name, package, arch, summary, desc):
        DebianGenerator.__init__(self, package, arch, summary, desc);
        self.name = name

    def _required_submodules(self):
        return None
    def _required_packages(self):
        return None

    def _makefile(self):
        self.required_submodules = "# SWITCHLIGHT_REQUIRED_SUBMODULES := "
        self.required_packages = "# SWITCHLIGHT_REQUIRED_PACKAGES := "
        if self._required_submodules():
            self.required_submodules = self.required_submodules[2:] + self._required_submodules()
        if self._required_packages():
            self.required_packages = self.required_packages[2:] + self._required_packages()

        """Return the contents of the top-level makefile for the component."""
        return """ifndef SWITCHLIGHT
$(error $$SWITCHLIGHT is undefined.)
endif

%(required_submodules)s
%(required_packages)s

include $(SWITCHLIGHT)/make/component.mk
""" % (self.__dict__)

    def _makefile_dot_comp_all_rules(self):
        return "\t@echo Nothing to be done."

    def _makefile_dot_comp(self):
        """Return the contents of the top-level Makefile.comp"""
        self.comp_all_rules = self._makefile_dot_comp_all_rules()
        return """# -*- Makefile -*-
############################################################
ifndef SWITCHLIGHT
$(error $$SWITCHLIGHT is not set)
endif

include $(SWITCHLIGHT)/make/config.mk

all:
%(comp_all_rules)s

.PHONY: deb
deb:
\t$(MAKE) -C deb
""" % (self.__dict__)

    def _deb_makefile(self):
        return """ARCH=%(arch)s
PACKAGE_NAMES=%(package)s
include %(relpath)s/../make/debuild.mk
""" % (self.__dict__)

    def __generate_file(self, path, name, contents):
        with open("%s/%s" % (path, name), "w") as f:
            f.write(contents)

    def generate(self, path):
        # Relative path to the SwitchLight root from the target package
        # directory
        location="%s/%s" % (path, self.name)
        os.makedirs(location)
        self.relpath = os.path.relpath(os.getenv('SWITCHLIGHT'),location)
        self.__generate_file(location, "Makefile", self._makefile())
        self.__generate_file(location, "Makefile.comp", self._makefile_dot_comp())
        location += "/deb"
        os.makedirs(location)
        self.__generate_file(location, "Makefile", self._deb_makefile())
        location += "/debuild"
        os.makedirs(location)
        DebianGenerator.generate(self, location)


if __name__ == "__main__":
    import argparse

    ap=argparse.ArgumentParser(description="Create a new component directory.")
    ap.add_argument("name", help="The name of the component.")
    ap.add_argument("arch", help="Package architecture.")
    ap.add_argument("summary", help="The package summary.")
    ap.add_argument("--package", help="The name of the package. Defaults to the name.")
    ap.add_argument("--desc", help="The package description.")

    ops = ap.parse_args()

    if ops.package is None:
        ops.package = ops.name
    if ops.desc is None:
        ops.desc = ops.summary

    cg = ComponentGenerator(ops.name, ops.package, ops.arch, ops.summary,
                            ops.desc)

    cg.generate(".")
