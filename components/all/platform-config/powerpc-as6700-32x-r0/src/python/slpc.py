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
############################################################
#
# Platform Driver for powerpc-as6700-32x-r0
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
        return 'powerpc-as6700-32x-r0'

    def _plat_info_dict(self):
        raise Exception()

    def _plat_oid_table(self):
        raise Exception()


if __name__ == "__main__":
    print SwitchLightPlatformImplementation()

