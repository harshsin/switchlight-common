2#!/usr/bin/python
############################################################
#
# Platform Driver for powerpc-as5710-54x-r0b
#
############################################################
import os
import struct
import time
import subprocess
from switchlight.platform.base import *
from switchlight.vendor.accton import *

class SwitchLightPlatformImplementation(SwitchLightPlatformAccton):

    def model(self):
        return "AS5710-54X"

    def platform(self):
        return 'powerpc-as5710-54x-r0b'

    def _plat_info_dict(self):
        return {
            platinfo.LAG_COMPONENT_MAX : 16,
            platinfo.PORT_COUNT : 54
            }

    def _plat_oid_table(self):
        return None

    def get_environment(self):
        return subprocess.check_output(['/usr/bin/ofad-ctl', 'environment'])



if __name__ == "__main__":
    print SwitchLightPlatformImplementation()
