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
# Platform Driver for the Quanta LY2
#
############################################################
import os
import struct
import time
import subprocess
from switchlight.platform.base import *
from switchlight.vendor.quanta import *
from switchlight.platform.sensors import *

class SwitchLightPlatformImplementation(SwitchLightPlatformQuanta):

    def _eeprom_file(self):
        return "/sys/devices/soc.0/ffe03000.i2c/i2c-0/i2c-2/2-0054/eeprom"

    def model(self):
        return "LY2"

    def platform(self):
        return "powerpc-quanta-ly2-r0"

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


    def psu_values(self, name, sensor):

        def filter_(f, info):
            if info['value'] in ['N/A', "FAULT" ]:
                return None

            if info['field'] == 'vout1':
                info['value'] = sensor_scale(info['value'], 500)

            return info


        values = sensor_values(sensor, 
                               dict(fan1= 'Fan  1', 
                                    temp1='Temp 1', 
                                    temp2='Temp 2', 
                                    temp3='Temp 3', 
                                    pin=  'Pin   ', 
                                    pout1='Pout  ', 
                                    iin=  'Iin   ', 
                                    iout1='Iout  ', 
                                    vin=  'Vin   ', 
                                    vout1='Vout  '), 
                               filter_=filter_, 
                               indent='    ')

        if values is None or len(values) == 0:
            return name + ': Not Present\n'
        else:
            return name + ':\n' + values


    def get_environment(self):

        s = ''

        s += 'System:\n'
        s += sensor_values('quanta_ly_hwmon-i2c-4-2e', 
                           dict(fan1= 'Fan  1',
                                fan2= 'Fan  2',
                                fan3= 'Fan  3',
                                fan4= 'Fan  4',
                                temp1='Temp 1',
                                temp2='Temp 2',
                                temp3='Temp 3',
                                temp4='Temp 4',
                                temp5='Temp 5'),
                           filter_=None,
                           indent='    ')

        s += self.psu_values('PSU 1', 'pmbus-i2c-6-58')
        s += self.psu_values('PSU 2', 'pmbus-i2c-7-59')

        return s
        


    def sys_init(self):
        subprocess.call("%s/sbin/gpio_init" % self.platform_basedir())


if __name__ == "__main__":
    import sys

    p = SwitchLightPlatformImplementation()
    if len(sys.argv) == 1 or sys.argv[1] == 'info':
        print p
    elif sys.argv[1] == 'env':
        print p.get_environment()



