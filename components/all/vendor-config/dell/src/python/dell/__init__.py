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
# SwitchLightPlatform support for DELL platforms.
#
############################################################
from switchlight.platform.base import SwitchLightPlatformBase, sysinfo
import struct
import time

class SwitchLightPlatformDELL(SwitchLightPlatformBase):

    def manufacturer(self):
        return "DELL"

    def _sys_info_dict(self):
        return {
            sysinfo.PRODUCT_NAME : "S4810"
            }

