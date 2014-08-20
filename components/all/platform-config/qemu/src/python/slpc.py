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
# Platform Driver for QEMU Simulation
#
############################################################
import os
import struct
import time
import subprocess
from switchlight.platform.base import *

class SwitchLightPlatformImplementation(SwitchLightPlatformBase):

    def model(self):
        return "QEMU"

    def manufacturer(self):
        return "QEMU"

    def platform(self):
        return "qemu"

    def _plat_info_dict(self):
        return {
            platinfo.LAG_COMPONENT_MAX : 8,
            platinfo.PORT_COUNT : 52
            }

    def _sys_info_dict(self):
        rv = {}
        rv[sysinfo.PRODUCT_NAME] = 'qemu'
        rv[sysinfo.SERIAL_NUMBER] = 'None'
        rv[sysinfo.MAC_ADDRESS] = '00:00:00:00:00:01'
        rv[sysinfo.PART_NUMBER] = 'qemu'
        return rv;

    def get_environment(self):
        return subprocess.check_output(['/usr/bin/ofad-ctl', 'environment'])


    def sys_init(self):
        pass


if __name__ == "__main__":
    print SwitchLightPlatformImplementation()

