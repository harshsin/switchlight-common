#!/usr/bin/python
############################################################
#
# Platform Driver for the Quanta LB9
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
        return "/sys/devices/e0000000.soc8541/e0003000.i2c/i2c-0/0-0053/eeprom"

    def model(self):
        return "LB9"

    def platform(self):
        return "powerpc-quanta-lb9-r0"

    def _plat_info_dict(self):
        return {
            platinfo.LAG_COMPONENT_MAX : 8,
            platinfo.PORT_COUNT : 52
            }

    def _plat_oid_table(self):
        return {
            oids.TEMP_SENSORS : {
                'ctemp1' : '.1.3.6.1.4.1.2021.13.16.2.1.3.1',
                'ctemp2' : '.1.3.6.1.4.1.2021.13.16.2.1.3.5',
                'ctemp3' : '.1.3.6.1.4.1.2021.13.16.2.1.3.9',
                'ctemp4' : '.1.3.6.1.4.1.2021.13.16.2.1.3.13',
                'ctemp5' : '.1.3.6.1.4.1.2021.13.16.2.1.3.17',
                'pwr-temp1' : '.1.3.6.1.4.1.2021.13.16.2.1.3.41',
                'pwr-temp2' : '.1.3.6.1.4.1.2021.13.16.2.1.3.44',
                'pwr-temp3' : '.1.3.6.1.4.1.2021.13.16.2.1.3.46',
                },
            oids.CHASSIS_FAN_SENSORS : {
                'cfan1' : '.1.3.6.1.4.1.2021.13.16.3.1.3.1',
                'cfan2' : '.1.3.6.1.4.1.2021.13.16.3.1.3.5',
                'cfan3' : '.1.3.6.1.4.1.2021.13.16.3.1.3.9',
                'cfan4' : '.1.3.6.1.4.1.2021.13.16.3.1.3.13',
                },
            oids.POWER_FAN_SENSORS : {
                'pwr-fan' : '.1.3.6.1.4.1.2021.13.16.3.1.3.33',
                },
            oids.POWER_SENSORS : {
                'power' : '.1.3.6.1.4.1.2021.13.16.5.1.3.8'
                },
            }

    def sys_init(self):
        subprocess.call("%s/sbin/gpio_init" % self.platform_basedir())


if __name__ == "__main__":
    print SwitchLightPlatformImplementation()

