#!/usr/bin/python
############################################################
#
# Platform Driver for the Quanta LY2
#
############################################################
import os
import struct
import time
import subprocess

class _SwitchLightPlatform(object):
    name="Quanta LY2"
    platform="quanta-ly2"
    manufacturer = "Quanta"
    num_phy_ports = 52
    lag_component_max=16


    # Cache of the system information read from the eeproms
    _sys_info = None

    @staticmethod
    def _sys_info_init():
        if _SwitchLightPlatform._sys_info is not None:
            return

        info = {}
        for e in ["/sys/devices/soc.0/ffe03000.i2c/i2c-0/i2c-2/2-0054/eeprom",
                  "/sys/devices/e0000000.soc8541/e0003000.i2c/i2c-0/0-0054/eeprom",
                  "/sys/devices/e0000000.soc8541/e0003000.i2c/i2c-0/0-0053/eeprom"]:
            try:
                f = file(e)
                if f.read(3) == "\xff\x01\xe0":
                    f = file(e)
                    break
            except IOError:
                pass
        else:
            return

        fields = {
            0xff: ("magic_num", "!B"),
            0x01: ("product_name", None),
            0x02: ("part_num", None),
            0x03: ("serial_num", None),
            0x04: ("mac_addr", "MAC"),
            0x05: ("mfg_date", "DATE"),
            0x06: ("card_type", "!L"),
            0x07: ("hw_version", "!L"),
            0x08: ("label_version", None),
            0x09: ("model_name", None),
            0x0a: ("sw_version", "!L"),
            0x00: ("crc16", "!H"),
            }

        while True:
            t = f.read(1)
            if not t:
                break
            t = ord(t)
            l = f.read(1)
            if not l:
                break
            l = ord(l)
            if l < 1:
                break
            x = f.read(l)
            if len(x) != l:
                break
            if t in fields:
                try:
                    t, c = fields[t]
                    if c == "MAC":
                        x = "%02x:%02x:%02x:%02x:%02x:%02x" % struct.unpack("!6B", x)
                    if c == "DATE":
                        x = time.gmtime(time.mktime(struct.unpack("!HBB", x)
                                                    + (0, 0, 0, 0, 0, 0)))
                    elif c:
                        x = struct.unpack(c, x)[0]
                except struct.error:
                    pass
            info[t] = x

        _SwitchLightPlatform._sys_info = info


    @staticmethod
    def sys_info(field):
        """Get a system information field."""
        _SwitchLightPlatform._sys_info_init()
        return _SwitchLightPlatform._sys_info.get(field)

    def port_config():
        return dict(max_port=52)

    @staticmethod
    def lag_config():
        return dict(max_lag=8)


    @staticmethod
    def oid_table():
        #
        # These keys must match the definitions in the cli's snmp.py
        # These should be standardized.
        #
        return {
            'temp_sensors' : {
                'ctemp1' : '.1.3.6.1.4.1.2021.13.16.2.1.3.1',
                'ctemp2' : '.1.3.6.1.4.1.2021.13.16.2.1.3.2',
                'ctemp3' : '.1.3.6.1.4.1.2021.13.16.2.1.3.3',
                'ctemp4' : '.1.3.6.1.4.1.2021.13.16.2.1.3.4',
                'ctemp5' : '.1.3.6.1.4.1.2021.13.16.2.1.3.5',
                'pwr-temp6' : '.1.3.6.1.4.1.2021.13.16.2.1.3.6',
                'pwr-temp7' : '.1.3.6.1.4.1.2021.13.16.2.1.3.9',
                'pwr-temp8' : '.1.3.6.1.4.1.2021.13.16.2.1.3.14',
                },
            'chassis_fan_sensors' : {
                'cfan1' : '.1.3.6.1.4.1.2021.13.16.3.1.3.1',
                'cfan2' : '.1.3.6.1.4.1.2021.13.16.3.1.3.2',
                'cfan3' : '.1.3.6.1.4.1.2021.13.16.3.1.3.3',
                'cfan4' : '.1.3.6.1.4.1.2021.13.16.3.1.3.4',
                },
            'power_fan_sensors' : {
                'pwr-fan' : '.1.3.6.1.4.1.2021.13.16.3.1.3.5',
                },
            'power_sensors' : {
                'power' : '.1.3.6.1.4.1.2021.13.16.5.1.3.1'
                },
            'CPU_load' : {
                'cpuload' : '.1.3.6.1.4.1.2021.10.1.5.1'
                },
            'mem_total_free': {
                'memtotalfree' : '.1.3.6.1.4.1.2021.4.11.0'
                }
            }

    @staticmethod
    def sys_hw_init():
        subprocess.call("/usr/bin/brcm_ly2_gpio_init")


if __name__ == "__main__":
    import pprint
    _SwitchLightPlatform._sys_info_init();
    pprint.pprint(_SwitchLightPlatform._sys_info)

