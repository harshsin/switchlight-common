############################################################
# <bsn.cl fy=2013 v=none>
# 
#        Copyright 2013, 2014 BigSwitch Networks, Inc.        
# 
# 
# 
# </bsn.cl>
############################################################
############################################################
#
# Installer scriptlet for the powerpc-as4600-54t
#

# The loader is installed in the fat partition of the first USB storage device
platform_bootcmd='usb start; fatload usb 0:1 0x10000000 switchlight-loader; setenv bootargs console=$consoledev,$baudrate sl_platform=powerpc-as4600-54t; bootm 0x10000000'

platform_installer() {
    # Standard installation to usb storage
    installer_standard_blockdev_install sda 16M 64M ""
}
