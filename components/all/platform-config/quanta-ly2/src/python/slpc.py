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
from switchlight.platform.base import *
from switchlight.vendor.quanta import *

class SwitchLightPlatformImplementation(SwitchLightPlatformQuanta):

    def _eeprom_file(self):
        return "/sys/devices/soc.0/ffe03000.i2c/i2c-0/i2c-2/2-0054/eeprom"

    def model(self):
        return "LY2"

    def platform(self):
        return "quanta-ly2"

    def _plat_info_dict(self):
        return {
            platinfo.LAG_COMPONENT_MAX : 16,
            platinfo.PORT_COUNT : 52
            }

    def _plat_oid_table(self):
        return {
            oids.TEMP_SENSORS : {
                'ctemp1' : '.1.3.6.1.4.1.2021.13.16.2.1.3.1',
                'ctemp2' : '.1.3.6.1.4.1.2021.13.16.2.1.3.2',
                'ctemp3' : '.1.3.6.1.4.1.2021.13.16.2.1.3.3',
                'ctemp4' : '.1.3.6.1.4.1.2021.13.16.2.1.3.4',
                'ctemp5' : '.1.3.6.1.4.1.2021.13.16.2.1.3.5',
                'pwr-temp6' : '.1.3.6.1.4.1.2021.13.16.2.1.3.6',
                'pwr-temp7' : '.1.3.6.1.4.1.2021.13.16.2.1.3.9',
                'pwr-temp8' : '.1.3.6.1.4.1.2021.13.16.2.1.3.14',
                },
            oids.CHASSIS_FAN_SENSORS : {
                'cfan1' : '.1.3.6.1.4.1.2021.13.16.3.1.3.1',
                'cfan2' : '.1.3.6.1.4.1.2021.13.16.3.1.3.2',
                'cfan3' : '.1.3.6.1.4.1.2021.13.16.3.1.3.3',
                'cfan4' : '.1.3.6.1.4.1.2021.13.16.3.1.3.4',
                },
            oids.POWER_FAN_SENSORS : {
                'pwr-fan' : '.1.3.6.1.4.1.2021.13.16.3.1.3.5',
                },
            oids.POWER_SENSORS : {
                'power' : '.1.3.6.1.4.1.2021.13.16.5.1.3.1'
                },
            }

    def sys_init(self):
        subprocess.call("%s/sbin/gpio_init" % self.platform_basedir())


if __name__ == "__main__":
    print SwitchLightPlatformImplementation()


