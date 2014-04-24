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
# Installer scriptlet for the powerpc-as5710-54x
#

# The loader must be written raw to the first partition.
platform_loader_raw=1
# The loader is installed in the fat partition of the first USB storage device

#
# Note -- setting the platform version as r0a is intentional here. We will use the r0a
# support as-is for r0b until the correct strategy for managing these things
# is implemented.
#

platform_bootcmd='usb start; usbboot 0x10000000 0:1; setenv bootargs console=$consoledev,$baudrate sl_platform=powerpc-as5710-54x-r0a; bootm 0x10000000'

platform_installer() {
    # Standard installation to usb storage
    installer_standard_blockdev_install sda 16M 64M ""
}