#! /bin/sh
############################################################
#
# The only argument is the SDK version suffix for the
# required modules.
#
############################################################
set -e

version=$1

if [ "${version}" = "" ]; then
    echo "usage: $0 <version>"
    exit 1
fi

# Remove old modules in case we're switching versions
[ -e /proc/linux-user-bde ] && rmmod linux-user-bde
[ -e /proc/linux-kernel-bde ] && rmmod linux-kernel-bde

# Install new modules
insmod /lib/modules/`uname -r`/linux-kernel-bde-${version}.ko
insmod /lib/modules/`uname -r`/linux-user-bde-${version}.ko

# Verify existance of the device file
[ -e /dev/linux-user-bde ] || mknod /dev/linux-user-bde c 126 0








