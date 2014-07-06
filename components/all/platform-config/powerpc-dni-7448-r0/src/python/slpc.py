#!/usr/bin/python
############################################################
#
# Platform Driver for powerpc-dni-7448-r0
#
############################################################
import os
import struct
import time
import subprocess
from switchlight.platform.base import *
from switchlight.vendor.dni import *

class SwitchLightPlatformImplementation(SwitchLightPlatformDNI):

    def model(self):
        return "7448"

    def platform(self):
        return 'powerpc-dni-7448-r0'

    def _plat_info_dict(self):
        return {
            platinfo.LAG_COMPONENT_MAX : 16,
            platinfo.PORT_COUNT : 52
            }

    def _plat_oid_table(self):
        return None

    def get_environment(self):
        return subprocess.check_output(['/usr/bin/ofad-ctl', 'environment'])



if __name__ == "__main__":
    from switchlight.platform.main import main
    main(SwitchLightPlatformImplementation())



