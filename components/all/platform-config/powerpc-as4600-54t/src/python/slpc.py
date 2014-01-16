#!/usr/bin/python
############################################################
#
# Platform Driver for powerpc-as4600-54t
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
        return 'AS4600-54T'

    def platform(self):
        return 'powerpc-as4600-54t'

    def _plat_info_dict(self):
        raise Exception()

    def _plat_oid_table(self):
        raise Exception()


if __name__ == "__main__":
    print SwitchLightPlatformImplementation()

