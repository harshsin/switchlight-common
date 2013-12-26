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
sys.path.append("/lib/platform-config/%s/python" % platform)

# Import the platform-specific class
from slpc import _SwitchLightPlatform

# Make it available to the importer as SwitchLightPlatform
SwitchLightPlatform=_SwitchLightPlatform




