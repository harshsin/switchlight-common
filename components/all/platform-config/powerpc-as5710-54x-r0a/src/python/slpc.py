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
        raise Exception()

    def platform(self):
        return 'powerpc-as5710-54x-r0a'

    def _plat_info_dict(self):
        raise Exception()

    def oid_table(self):
        raise Exception()


if __name__ == "__main__":
    print SwitchLightPlatformImplementation()

