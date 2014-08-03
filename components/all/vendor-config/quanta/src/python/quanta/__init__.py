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
# SwitchLightPlatform support for Quanta platforms.
#
############################################################
from switchlight.platform.base import SwitchLightPlatformBase, sysinfo
import struct
import time
import os
import json

class SwitchLightPlatformQuanta(SwitchLightPlatformBase):


    def manufacturer(self):
        return "Quanta"

    def _sys_info_dict_onie(self):
        if os.path.exists("/etc/.onie.json"):
            od = json.load(file("/etc/.onie.json"))
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

    def _eeprom_file(self):
        return None

    def _sys_info_dict(self):

        if self._eeprom_file() is None:
            return self._sys_info_dict_onie()

        info = {}

        # _eeprom_file() is provided by the derived platform class
        f = file(self._eeprom_file())

        try:
            if f.read(3) != "\xff\x01\xe0":
                # Magic number does not match.
                return None
        except IOError:
            return None

        f = file(self._eeprom_file())

        #
        # EEPROM data mappings.
        # codebyte : (field, unpack-size)
        #
        fields = {
            0xff: (sysinfo.MAGIC, "!B"),
            0x01: (sysinfo.PRODUCT_NAME, None),
            0x02: (sysinfo.PART_NUMBER, None),
            0x03: (sysinfo.SERIAL_NUMBER, None),
            0x04: (sysinfo.MAC_ADDRESS, "MAC"),
            0x05: (sysinfo.MANUFACTURE_DATE, "DATE"),
            0x06: (sysinfo.CARD_TYPE, "!L"),
            0x07: (sysinfo.HARDWARE_VERSION, "!L"),
            0x08: (sysinfo.LABEL_VERSION, None),
            0x09: (sysinfo.MODEL_NAME, None),
            0x0a: (sysinfo.SOFTWARE_VERSION, "!L"),
            0x00: (sysinfo.CRC16, "!H"),
            }

        while True:
            # Read type codebyte
            t = f.read(1)
            if not t:
                break
            t = ord(t)
            # Read length
            l = f.read(1)
            if not l:
                break
            l = ord(l)
            if l < 1:
                break
            # Read value
            v = f.read(l)
            if len(v) != l:
                break
            # Populate field by type
            if t in fields:
                try:
                    t, c = fields[t]
                    if c == "MAC":
                        v = "%02x:%02x:%02x:%02x:%02x:%02x" % struct.unpack("!6B", v)
                    if c == "DATE":
                        v = time.gmtime(time.mktime(struct.unpack("!HBB", v)
                                                    + (0, 0, 0, 0, 0, 0)))
                        v = time.strftime("%a, %d %b %Y %H:%M:%S +0000", v)
                    elif c:
                        v = struct.unpack(c, v)[0]
                except struct.error:
                    pass

                info[t] = v

        return info



