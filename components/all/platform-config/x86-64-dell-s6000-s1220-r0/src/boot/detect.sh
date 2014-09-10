############################################################
# <bsn.cl fy=2013 v=none>
#
#        Copyright 2013, 2014 BigSwitch Networks, Inc.
#
#
#
# </bsn.cl>
############################################################
# Default platform detection.
if grep -q "QEMU Virtual CPU" /proc/cpuinfo; then
    echo "x86-64-dell-s6000-s1220-r0" >/etc/sl_platform
    exit 0
else
    exit 1
fi

