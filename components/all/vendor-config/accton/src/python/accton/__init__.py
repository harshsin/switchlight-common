#!/usr/bin/python
############################################################
#
# SwitchLightPlatform support for Accton platforms.
#
############################################################
from switchlight.platform.base import SwitchLightPlatformBase, sysinfo
import struct
import time

class SwitchLightPlatformAccton(SwitchLightPlatformBase):

    def manufacturer(self):
        return "Accton"

    def _sys_info_dict(self):
        return {
            sysinfo.PRODUCT_NAME : "AcctonNotImplemented",
            }

