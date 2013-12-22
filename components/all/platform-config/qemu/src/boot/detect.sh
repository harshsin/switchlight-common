# Default platform detection.
if grep -q "QEMU Virtual CPU" /proc/cpuinfo; then
    echo "qemu" >/etc/sl_platform
    exit 0
else
    exit 1
fi

