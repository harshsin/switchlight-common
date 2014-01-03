#!/usr/bin/python
############################################################
#
# This file provides the container for the
# platform-specific class provided by the files in the
# platform-config packages.
#
############################################################

# Determine the platform
with open("/etc/sl_platform", 'r') as f:
    platform=f.read().strip()

# Append platform-specific paths for import
import sys
platform_basedir="/lib/platform-config/%s" % platform
sys.path.append("%s/python" % platform_basedir)

# Import the platform-specific class
from slpc import SwitchLightPlatformImplementation

# Make it available to the importer as SwitchLightPlatform
SwitchLightPlatform=SwitchLightPlatformImplementation

if __name__ == "__main__":
    print SwitchLightPlatform()





