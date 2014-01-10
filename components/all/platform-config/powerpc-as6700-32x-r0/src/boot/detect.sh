# Default platform detection.
if grep -q "^model.*: powerpc-as6700-32x-r0$" /proc/cpuinfo; then
    echo "powerpc-as6700-32x-r0" >/etc/sl_platform
    exit 0
else
    exit 1
fi

