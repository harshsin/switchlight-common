#!/usr/bin/python
############################################################
#
# Platform Driver for powerpc-as5710-54x-r0a
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
        return "Accton AS5710-54X"

    def platform(self):
        return 'powerpc-as5710-54x-r0a'

    def _plat_info_dict(self):
        return {
            platinfo.LAG_COMPONENT_MAX : 16,
            platinfo.PORT_COUNT : 54
            }

    def _plat_oid_table(self):
        return None

    def get_environment(self):
        return "Not implemented."



if __name__ == "__main__":
    from switchlight.platform.main import main
    main(SwitchLightPlatformImplementation())



