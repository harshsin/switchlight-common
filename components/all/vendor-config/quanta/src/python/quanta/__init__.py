#!/usr/bin/python
############################################################
#
# SwitchLightPlatform support for Quanta platforms.
#
############################################################
from switchlight.platform.base import SwitchLightPlatformBase, sysinfo
import struct
import time

class SwitchLightPlatformQuanta(SwitchLightPlatformBase):

    def manufacturer(self):
        return "Quanta"

    def _sys_info_dict(self):
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



