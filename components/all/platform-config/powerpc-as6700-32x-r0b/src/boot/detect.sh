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
if grep -q "^model.*: powerpc-as6700-32x-r0b$" /proc/cpuinfo; then
    echo "powerpc-as6700-32x-r0b" >/etc/sl_platform
    exit 0
else
    exit 1
fi

