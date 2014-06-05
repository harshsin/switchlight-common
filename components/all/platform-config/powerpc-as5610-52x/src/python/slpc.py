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
# Platform Driver for powerpc-as5610-52x
#
############################################################
import os
import struct
import time
import subprocess
from switchlight.platform.base import *
from switchlight.vendor.accton import *

class SwitchLightPlatformImplementation(SwitchLightPlatformAccton):

    onie_base_address = "0xeff70000"

    def model(self):
        return "AS5610-52X"

    def platform(self):
        return 'powerpc-as5610-52x'

    def _plat_info_dict(self):
        return {
            platinfo.LAG_COMPONENT_MAX : 16,
            platinfo.PORT_COUNT : 52,
            }

    def _plat_oid_table(self):
        BSN_SENSORS = '.1.3.6.1.4.1.37538.2.3'
        return {
            oids.TEMP_SENSORS : {
                'ct1'   : BSN_SENSORS + '.1.1.3.1',
                'ct2'   : BSN_SENSORS + '.1.1.3.2',
                'ct3'   : BSN_SENSORS + '.1.1.3.3',
                'ct4'   : BSN_SENSORS + '.1.1.3.4',
                'ct5'   : BSN_SENSORS + '.1.1.3.5',
                'ct6'   : BSN_SENSORS + '.1.1.3.6',
                'ct7'   : BSN_SENSORS + '.1.1.3.7',
                'ct8'   : BSN_SENSORS + '.1.1.3.8',
                'ct9'   : BSN_SENSORS + '.1.1.3.9',
                },
            oids.CHASSIS_FAN_SENSORS : {
                'cfan1' : BSN_SENSORS + '.2.1.3.2',
                },
            oids.POWER_FAN_SENSORS : {
                'pfan1' : BSN_SENSORS + '.2.1.3.1',
                'pfan2' : BSN_SENSORS + '.2.1.3.3',
                },
            oids.POWER_SENSORS : {
                'power' : BSN_SENSORS + '.4.1.3.3',
                },
            }


    def get_environment(self):
        return subprocess.check_output(['/usr/bin/ofad-ctl', 'environment'])


if __name__ == "__main__":
    print SwitchLightPlatformImplementation()

