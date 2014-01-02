######################################################################
# 
# Installer scriptlet for the Quanta LY2. 
#    

# The bootcommand is to read the loader directly from the first partition and execute it. 
platform_bootcmd='mmc part 0; fatload mmc 0:1 0x10000000 switchlight-loader; setenv bootargs console=$consoledev,$baudrate sl_platform=powerpc-quanta-ly2-r0; bootm 0x10000000'

platform_installer() {
    # Standard installation on the CF card.
    installer_standard_blockdev_install mmcblk0 16M 64M ""
}
