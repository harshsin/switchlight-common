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
# Platform Driver for powerpc-as6700-32x-r0b
#
############################################################
import os
import struct
import time
import subprocess
from switchlight.platform.base import *
from switchlight.vendor.accton import *

class SwitchLightPlatformImplementation(SwitchLightPlatformAccton):

    def model(self):
        return "Accton AS6700-32X"

    def platform(self):
        return 'powerpc-as6700-32x-r0b'

    def _plat_info_dict(self):
        return {
            platinfo.LAG_COMPONENT_MAX : 16,
            platinfo.PORT_COUNT : 32
            }

    def _plat_oid_table(self):
        raise Exception()

    def get_environment(self):
        return "Not implemented."


if __name__ == "__main__":
    from switchlight.platform.main import main
    main(SwitchLightPlatformImplementation())

