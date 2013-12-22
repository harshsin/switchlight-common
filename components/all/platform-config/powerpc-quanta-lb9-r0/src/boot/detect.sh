# Default platform detection.
if grep -q "^model.*: powerpc-quanta-lb9-r0$" /proc/cpuinfo; then
    echo "powerpc-quanta-lb9-r0" >/etc/sl_platform
    exit 0
else
    exit 1
fi

