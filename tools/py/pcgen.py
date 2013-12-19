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
    def __init__(self, platform):
        self.platform = platform
        summary="Platform Configuration files for %s." % self.platform
        ComponentGenerator.__init__(self, platform, "platform-config-" + self.platform, 
                                    "all", summary, summary)
        
    def _makefile_dot_comp_all_rules(self):
        return "\t@echo Run 'make deb'"

    def _dh_auto_install(self):
        return """\tmkdir -p $(INSTALL_DIR)/usr/platform-config/
\tcp -R ../../src $(INSTALL_DIR)/usr/platform-config/"""

    def __generate_file(self, path, name, contents):
        with open("%s/%s" % (path, name), "w") as f:
            f.write(contents)
            
    def _install(self):
        return "/usr/platform-config/*"

    def generate(self, path):
        # Generate the entire component:
        ComponentGenerator.generate(self, path)
        self.path = "%s/%s" % (path, self.platform)
        # the platform directory layout is this
        os.makedirs('%(path)s/src/installer/%(platform)s' % (self.__dict__))
        os.makedirs('%(path)s/src/loader' % (self.__dict__))

        self.__generate_file('%(path)s/src' % self.__dict__, 'name', self.platform+'\n')
        self.__generate_file('%(path)s/src/installer/%(platform)s' % self.__dict__, 
                             '%(platform)s.sh' % self.__dict__,
                             "# Platform data goes here.")
        self.__generate_file('%(path)s/src/loader' % self.__dict__, 
                             self.platform, 
                             "# Platform data goes here.")
        

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print "usage: %s <platform-name>" % sys.argv[0]
        sys.exit(1)
        
    pc = PlatformConfigGenerator(sys.argv[1])
    pc.generate('.')


    
