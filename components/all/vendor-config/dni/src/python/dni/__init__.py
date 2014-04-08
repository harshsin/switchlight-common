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
# SwitchLightPlatform support for DNI platforms.
#
############################################################
from switchlight.platform.base import SwitchLightPlatformBase, sysinfo
import struct
import time

class SwitchLightPlatformDNI(SwitchLightPlatformBase):

    def manufacturer(self):
        return "DNI"

    def _sys_info_dict(self):
        return {
            sysinfo.PRODUCT_NAME : "DNINotImplemented",
            }

# Vendor classes here
