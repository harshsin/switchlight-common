############################################################
#
# Installer scriptlet for the powerpc-as6700-32x
#

# The loader is installed in the fat partition of the first USB storage device
platform_bootcmd='usb start; fatload usb 0:1 0x10000000 switchlight-loader; setenv bootargs console=$consoledev,$baudrate sl_platform=powerpc-as6700-32x-r0; bootm 0x10000000'

platform_installer() {
    # Standard installation to usb storage
    installer_standard_blockdev_install sda 16M 64M ""
}