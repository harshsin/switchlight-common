######################################################################
# 
# Installer scriptlet for the Quanta LB9. 
#    
# The loader must be written raw to the first partition. 
platform_loader_raw=1
# The bootcommand is to read the loader directly from the first partition and execute it. 
platform_bootcmd='diskboot 0x10000000 0:1 ; setenv bootargs console=$consoledev,$baudrate ; bootm 0x10000000'

platform_installer() {
    # Standard installation on the CF card.
    installer_standard_install sda 16M 64M ""
}

