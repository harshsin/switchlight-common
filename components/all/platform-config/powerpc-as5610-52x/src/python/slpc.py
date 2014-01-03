#!/usr/bin/python
############################################################
#
# Platform Driver for powerpc-as5610-52x
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
        return "AS5610-52X"

    def platform(self):
        return 'powerpc-as5610-52x'

    def _plat_info_dict(self):
        return {
            platinfo.LAG_COMPONENT_MAX : 16,
            platinfo.PORT_COUNT : 52,
            }

    def oid_table(self):
        raise Exception()


if __name__ == "__main__":
    print SwitchLightPlatformImplementation()

