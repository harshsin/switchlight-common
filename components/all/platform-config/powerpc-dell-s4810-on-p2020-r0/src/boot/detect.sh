# Default platform detection.
if grep -q "^model.*: powerpc-dell-s4810-on-p2020-r0$" /proc/cpuinfo; then
    echo "powerpc-dell-s4810-on-p2020-r0" >/etc/sl_platform
    exit 0
else
    exit 1
fi

