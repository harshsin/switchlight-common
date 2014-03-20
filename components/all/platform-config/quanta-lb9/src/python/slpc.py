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
# Platform Driver for the Quanta LB9
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
        return "/sys/devices/e0000000.soc8541/e0003000.i2c/i2c-0/0-0053/eeprom"

    def model(self):
        return "LB9"

    def platform(self):
        return "quanta-lb9"

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


    def get_environment(self):
        
        def filter_(f, info):
            if info['value'] in ['N/A', "FAULT" ]:
                return None

            if info['field'] == 'vout1':
                info['value'] = sensor_scale(info['value'], 500)
            
            return info

        s = ''

        s += 'System:\n'
        s += sensor_values('adt7470-i2c-5-2c', 
                           dict(fan1= 'Fan  1', 
                                fan2= 'Fan  2', 
                                fan3= 'Fan  3', 
                                fan4= 'Fan  4', 
                                temp1='Temp 1', 
                                temp2='Temp 2', 
                                temp3='Temp 3',
                                temp4='Temp 4', 
                                temp5='Temp 5'), 
                           filter_=filter_, 
                           indent='    ')
        
        psu1 = sensor_values('pmbus-i2c-7-58', 
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
        if psu1 is None or len(psu1) == 0:
            s += 'PSU 1: Not Present\n'
        else:
            s += 'PSU 1:\n' + psu1

        psu2 = sensor_values('pmbus-i2c-8-59', 
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
        if psu2 is None or len(psu2) == 0:
            s += 'PSU 2: Not Present\n'
        else:
            s += 'PSU 2:\n' + psu2

        return s

        
        



if __name__ == "__main__":
    import sys

    p = SwitchLightPlatformImplementation()
    if len(sys.argv) == 1 or sys.argv[1] == 'info':
        print p
    elif sys.argv[1] == 'env':
        print p.get_environment()
