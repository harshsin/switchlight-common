# Default platform detection.
if grep -q "^model.*: powerpc-as4600-54t$" /proc/cpuinfo; then
    echo "powerpc-as4600-54t" >/etc/sl_platform
    exit 0
else
    exit 1
fi

