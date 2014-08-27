#!/usr/bin/python
############################################################
# <bsn.cl fy=2013 v=none>
#
#        Copyright 2013, 2014 BigSwitch Networks, Inc.
#
#
#
# </bsn.cl>
############################################################
#
# Platform Driver for the Quanta LY2
#
############################################################
import os
import struct
import time
import subprocess
from switchlight.platform.base import *
from switchlight.vendor.quanta import *
from switchlight.platform.sensors import *

class SwitchLightPlatformImplementation(SwitchLightPlatformQuanta):

    def model(self):
        return "LY2"

    def platform(self):
        return "powerpc-quanta-ly2-r0"

    def _plat_info_dict(self):
        return {
            platinfo.LAG_COMPONENT_MAX : 16,
            platinfo.PORT_COUNT : 52
            }

    def get_environment(self):
        return subprocess.check_output(['/usr/bin/ofad-ctl', 'environment'])

    def sys_init(self):
        subprocess.call("%s/sbin/gpio_init" % self.platform_basedir())


if __name__ == "__main__":
    print SwitchLightPlatformImplementation()




