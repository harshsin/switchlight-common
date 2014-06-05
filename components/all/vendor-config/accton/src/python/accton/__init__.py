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
# SwitchLightPlatform support for Accton platforms.
#
############################################################
from switchlight.platform.base import SwitchLightPlatformBase, sysinfo
import struct
import time
import json
import os

class SwitchLightPlatformAccton(SwitchLightPlatformBase):

    onie_base_address = None

    def manufacturer(self):
        return "Accton"

    def _sys_info_dict(self):
        if self.onie_base_address is None:
            return None
        else:
            if not os.path.exists("/etc/onie.json"):
                os.system("/sbin/mtool odump %s json > /etc/onie.json" % self.onie_base_address);
            if os.path.exists("/etc/onie.json"):
                od = json.load(file("/etc/onie.json"))
                rv = {}
                rv[sysinfo.PRODUCT_NAME] = od['Product Name']
                rv[sysinfo.SERIAL_NUMBER] = od['Serial Number']
                if rv[sysinfo.SERIAL_NUMBER] is None:
                    rv[sysinfo.SERIAL_NUMBER] = "UNKNOWN"
                rv[sysinfo.MAC_ADDRESS] = od['MAC']
                rv[sysinfo.PART_NUMBER] = od['Part Number']
                rv[sysinfo.MANUFACTURE_DATE] = od['Manufacture Date']
                rv[sysinfo.HARDWARE_VERSION] = od['Label Revision']
                return rv;
            else:
                return None

