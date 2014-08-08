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

import pprint

############################################################
#
# System-specific information keys.
# These represent information about a particular box.
#
############################################################

class sysinfo(object):
    MAGIC='_magic'
    CRC16='_crc16'
    PRODUCT_NAME='Product Name'
    PART_NUMBER='Part Number'
    SERIAL_NUMBER='Serial Number'
    MAC_ADDRESS='MAC Address'
    MANUFACTURE_DATE='Manufactured Date'
    CARD_TYPE='Card Type'
    HARDWARE_VERSION='Hardware Version'
    LABEL_VERSION='Label Version'
    MODEL_NAME='Model'
    SOFTWARE_VERSION='Software Version'

############################################################
#
# Platform-specific information keys.
# These represent information about a particular type of box.
#
############################################################
class platinfo(object):
    LAG_COMPONENT_MAX='Maximum number of component ports in a LAG'
    PORT_COUNT='Total Physical Ports'

############################################################
#
# Symbolic port base
#
############################################################
class basenames(object):
    PHY="ethernet"
    LAG="port-channel"

############################################################
#
# SwitchLight Platform Base
# Baseclass for all SwitchLightPlatform objects.
#
############################################################

class SwitchLightPlatformBase(object):

    def __init__(self):
        self.sys_info = None

    def platform_basedir(self):
        return "/lib/platform-config/%s" % self.platform()

    def manufacturer(self):
        raise Exception("Manufacturer is not set.")

    def model(self):
        raise Exception("Model is not set.")

    def platform(self):
        raise Exception("Platform is not set.")

    def description(self):
        return "%s %s (%s)" % (self.manufacturer(), self.model(),
                               self.platform())

    def serialnumber(self):
        return self.sys_info_get(sysinfo.SERIAL_NUMBER)

    def hw_description(self):
        return "%s (%s)" % (self.sys_info_get(sysinfo.PRODUCT_NAME),
                            self.sys_info_get(sysinfo.PART_NUMBER))


    def sys_object_id(self):
        return "1.3.6.1.4.1.37538.2"

    def ifnumber(self):
        #
        # The default assumption for any platform
        # is ma1, lo, and all front-panel ports.
        #
        return self.portcount() + 2

    def portcount(self):
        return self.plat_info_get(platinfo.PORT_COUNT)


    def __getattr__(self, key):
        class __InfoContainer(object):
            def __init__(self, d, klass):
                # Set all known info keys to None
                for (m,n) in klass.__dict__.iteritems():
                    if m == m.upper():
                        setattr(self, m, None)
                if d:
                    for (k,v) in d.iteritems():
                        for (m,n) in klass.__dict__.iteritems():
                            if n == k:
                                setattr(self, m, v);
                                break
        if key == "platinfo":
            return __InfoContainer(self.plat_info_get(), platinfo)
        if key == "sysinfo":
            return __InfoContainer(self.sys_info_get(), sysinfo)
        return None

    def _sys_info_dict(self):
        raise Exception("Must be provided by the deriving class.")

    def _plat_info_dict(self):
        raise Exception("Must be provided by the deriving class.")

    def _plat_oid_table(self):
        raise Exception("Must be provided by the deriving class.")

    def get_environment(self):
        raise Exception("Must be provided by the deriving class.")

    def sys_info_get(self, field=None):
        """Provide the value of a sysinfo key or the entire dict"""
        if self.sys_info is None:
            self.sys_info = self._sys_info_dict()

        if self.sys_info:
            if field:
                return self.sys_info.get(field)
            else:
                return self.sys_info
        else:
            return {}

    def plat_info_get(self, field=None):
        """Provide the value of a platinfo key or the entire dict"""
        pi = self._plat_info_dict()
        if field and pi:
            return pi.get(field)
        else:
            return pi

    def sys_init(self):
        """Optional system initialization."""
        return True

    def __infostr(self, d, indent="    "):
        """String representation of a platform information dict."""
        # A little prettier than pprint.pformat(), especially
        # if we are displaying the infromation to the user.
        # We also want to hide keys that start with an underscore.
        return "\n".join( sorted("%s%s: %s" % (indent,k,v) for k,v in d.iteritems() if not k.startswith('_')))

    def __str__(self):
        return """Manufacturer: %s
Model: %s
Platform: %s
Description: %s
System Information:
%s

Platform Information:
%s

""" % (self.manufacturer(),
       self.model(),
       self.platform(),
       self.description(),
       self.__infostr(self.sys_info_get()),
       self.__infostr(self.plat_info_get()),
       )




