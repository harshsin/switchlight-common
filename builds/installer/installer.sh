#!/bin/sh

cd $(dirname $0)

# Remount tmpfs larger
mount -oremount,size=512M /tmp

# Pick up defines for this machine.
[ -r /etc/machine.conf ] && . /etc/machine.conf

if [ "${onie_platform}" ]; then
    say() {
        echo "$@" >/dev/console
# Only echo to console for now
#        echo "$@"
    }
    trap 'say "Install failed!"; cat /var/log/onie.log >/dev/console; say "Install failed! See log messages above for details"; sleep 3; reboot' EXIT
    platform=${onie_platform}
else
    say() {
        echo
        echo "* $@"
        echo
    }
    trap 'say "Install failed!"; exit 1' EXIT
    platform=$(cat /etc/sl_platform)

    case "${platform}" in
        quanta-lb9)
            platform=powerpc-quanta_lb9-r0
            ;;
        quanta-lb9a)
            platform=powerpc-quanta_lb9a-r0
            ;;
        quanta-ly2)
            platform=powerpc-quanta_ly2-r0
            ;;
        quanta-ly5)
            platform=powerpc-quanta_ly5-r0
            ;;
    esac
fi

say "Switch Light install for ${platform}"

case "${platform}" in
    powerpc-quanta_lb9-r0)
        flashimage=switchlight.quanta-lb9.flash
        flashdev=/dev/mtd0
        loader1dev=mtd0
        loader1addr=0xfe000000
        loader2dev=mtd1
        loader2addr=0xfe800000
        swi=switchlight-powerpc.swi
        flash2dev=/dev/sda
        flash2fsdev=/dev/sda1
        ;;
    powerpc-quanta_lb9a-r0)
        flashimage=switchlight.quanta-lb9a.flash
        flashdev=/dev/mtd0
        loader1dev=mtd0
        loader1addr=0xfe000000
        loader2dev=mtd1
        loader2addr=0xfe800000
        swi=switchlight-powerpc.swi
        flash2dev=/dev/sda
        flash2fsdev=/dev/sda1
        ;;
    powerpc-quanta_ly2-r0)
        flashimage=switchlight.quanta-ly2.flash
        flashdev=/dev/mtd0
        loader1dev=mtd0
        loader1addr=0xec000000
        loader2dev=mtd1
        loader2addr=0xed000000
        swi=switchlight-powerpc.swi
        flash2dev=/dev/mmcblk0
        flash2fsdev=/dev/mmcblk0p1
        ;;
    powerpc-quanta_ly5-r0)
        flashimage=switchlight.quanta-ly5.flash
        flashdev=/dev/mtd0
        loader1dev=mtd0
        loader1addr=0xe8000000
        loader2dev=mtd1
        loader2addr=0xe9000000
        swi=switchlight-powerpc.swi
        flash2dev=/dev/mmcblk0
        flash2fsdev=/dev/mmcblk0p1
        ;;
    dni_7448)
        flashimage=switchlight.delta-7448.flash
        flashdev=/dev/mtd0
        loader1dev=mtd0
        loader1addr=0xe8000000
        loader2dev=mtd1
        loader2addr=0xe9000000
        swi=switchlight-powerpc.swi
        flash2dev=/dev/mmcblk0
        flash2fsdev=/dev/mmcblk0p1
        ;;
    celestica-redstone)
        flashimage=switchlight.celestica-redstone.flash
        flashdev=/dev/mtd0
        loader1dev=mtd0
        loader1addr=0xec000000
        loader2dev=mtd1
        loader2addr=0xed000000
        swi=switchlight-powerpc.swi
        flash2dev=
        flash2fsdev=
        ;;
    *)
        say "This installer does not support platform ${platform}"
        exit 1
        ;;
esac

if [ "${onie_platform}" ]; then
    echo >/dev/console
    echo >/dev/console
fi
say "Unpacking Switch Light installer"

rm -rf /tmp/.installer
mkdir /tmp/.installer
sed -e '1,/^PAYLOAD_FOLLOWS$/d' "$0" | tar -C /tmp/.installer -x -z || exit 1

set -e

if [ "${flash2dev}" ]; then
    if [ -e "${flash2dev}" ]; then
        say "Installing Switch Light software image"

        umount -l /tmp/.fs 2>/dev/null || :
        umount -l ${flash2fsdev} 2>/dev/null || :
        rm -rf /tmp/.fs
        mkdir /tmp/.fs
        dd if=/dev/zero of=${flash2dev} bs=16M count=1
        echo -e 'o\nn\np\n1\n\n\nt\n83\np\nw\n' | fdisk ${flash2dev}
        mdev -s # trigger re-creating partition device node
        mkdosfs -F 32 -v ${flash2fsdev}
        mount -t vfat ${flash2fsdev} /tmp/.fs
        cp /tmp/.installer/${swi} /tmp/.fs/
        umount /tmp/.fs
    else
        say "Flash device ${flash2dev} not found; skipping software image install"
    fi
fi

say "Installing Switch Light loader (takes several minutes)"

flashcp -v /tmp/.installer/${flashimage} ${flashdev}

bootcmd="setenv bootargs console=\$consoledev,\$baudrate sl_loader=${loader1dev}; bootm ${loader1addr}; setenv bootargs console=\$consoledev,\$baudrate sl_loader=${loader2dev}; bootm ${loader2addr}"

if [ "${onie_platform}" ]; then
    say "Setting boot command to boot Switch Light"
    echo "nos_bootcmd ${bootcmd}" >/tmp/.env
    fw_setenv -f -s /tmp/.env
    trap - EXIT
    say "Install finished.  Rebooting to Switch Light."
    sleep 3
    reboot
else
    trap - EXIT
    say "Install finished."
    echo "To configure U-Boot to boot Switch Light automatically, reboot the switch,"
    echo "enter the U-Boot shell, and run these 2 commands:"
    echo "=> setenv bootcmd '${bootcmd}'"
    echo "=> saveenv"
fi

exit

PAYLOAD_FOLLOWS