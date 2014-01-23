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
if grep -q "^model.*: powerpc-quanta-ly2-r0$" /proc/cpuinfo; then
    echo "powerpc-quanta-ly2-r0" >/etc/sl_platform
    exit 0
else
    exit 1
fi

