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
# Installer scriptlet for the powerpc-as6700-32x
#
if fw_printenv sl_installer_use_mmc; then
    # The loader is installed in the fat partition of the first MMC device.
    # This is a workaround for early samples that had a broken flash device.
    # It requires manually inserted an SD card.
    # This cannot be used in production.
    platform_bootcmd='mmc part 0; fatload mmc 0:1 0x10000000 switchlight-loader; setenv bootargs console=$consoledev,$baudrate sl_platform=powerpc-as6700-32x-r0; bootm 0x10000000'
    __blockdev=mmcblk0
else
    # The loader is installed in the fat partition of the first USB storage device
    platform_bootcmd='usb start; fatload usb 0:1 0x10000000 switchlight-loader; setenv bootargs console=$consoledev,$baudrate sl_platform=powerpc-as6700-32x-r0; bootm 0x10000000'
    __blockdev=sda
fi

platform_installer() {
    # Standard installation to usb storage
    installer_standard_blockdev_install "${__blockdev}" 16M 64M ""

    #
    # Hack die die
    # Get the mac address currently used by ONIE
    local macaddr=$(ifconfig eth0 | awk '/HWaddr/ { print tolower($5) }')

    # We need to set eth2addr and eth3addr in order for our interfaces to come up.
    # eth2addr is unused. eth3addr is ma1.
    # Fixme.
    fw_setenv -f eth2addr 00:00:00:FF:FF:FF
    fw_setenv -f eth3addr $macaddr
}