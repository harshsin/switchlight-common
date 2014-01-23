#!/bin/sh
############################################################
# <bsn.cl fy=2013 v=none>
# 
#        Copyright 2013, 2014 BigSwitch Networks, Inc.        
# 
# 
# 
# </bsn.cl>
############################################################

############################################################
#
# THIS INSTALLER IS DEPRECATED AND NO LONGER USED OR 
# SUPPORTED. 
#
############################################################

set -e 

cd $(dirname $0)

# Installer debug option from the environment
fw_printenv sl_installer_debug &> /dev/null && debug=1

if [ "$debug" ]; then
    set -x
fi


# Remount tmpfs larger if possible
mount -oremount,size=512M /tmp || true

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
        quanta-ly2r)
            platform=powerpc-quanta_ly2r-r0
            ;;
        quanta-ly5)
            platform=powerpc-quanta_ly5-r0
	    ;;
	accton-5651)
	    platform=powerpc-es5652bt1-flf-zz-r0
            ;;
    esac
fi

sl_version="@SLVERSION@"

say "Switch Light install for ${platform}"
say "[from ${sl_version}]"

case "${platform}" in
    powerpc-quanta_lb9-r0)
        # loader and /mnt/flash share first partition,
        # will be re-partitioned when loader boots
        # XXX roth -- untested
        flashimage=switchlight.quanta-lb9.flash
        flashdev=/dev/mtd0
        loaderdev=mtd0
        loaderaddr=0xfe000000
        jffs2dev=
        jffs2image=
        swi=switchlight-powerpc.swi
        flash2dev=/dev/sda
        flash2fsdev=/dev/sda1
        ;;
    powerpc-quanta_lb9a-r0)
        # loader and /mnt/flash share first partition,
        # will be re-partitioned when loader boots
        # XXX roth -- untested
        flashimage=switchlight.quanta-lb9a.flash
        flashdev=/dev/mtd0
        loaderdev=mtd0
        loaderaddr=0xfe000000
        jffs2dev=
        jffs2image=
        swi=switchlight-powerpc.swi
        flash2dev=/dev/sda
        flash2fsdev=/dev/sda1
        ;;
    powerpc-quanta_ly2-r0)
        flashimage=switchlight.quanta-ly2.loader
        flashdev=/dev/mtd0
        loaderdev=mtd0
        loaderaddr=0xee000000
        jffs2dev=/dev/mtd4
        jffs2image=switchlight.quanta-ly2.jffs2
        swi=switchlight-powerpc.swi
        flash2dev=/dev/mmcblk0
        flash2fsdev=/dev/mmcblk0p1
        ;;
    powerpc-quanta_ly2r-r0)
        flashimage=switchlight.quanta-ly2r.loader
        flashdev=/dev/mtd0
        loaderdev=mtd0
        loaderaddr=0xee000000
        jffs2dev=/dev/mtd4
        jffs2image=switchlight.quanta-ly2r.jffs2
        swi=switchlight-powerpc.swi
        flash2dev=/dev/mmcblk0
        flash2fsdev=/dev/mmcblk0p1
        ;;
    powerpc-quanta_ly5-r0)
        # XXX roth -- untested, possibly two or more flash parts
        flashimage=switchlight.quanta-ly5.flash
        flashdev=/dev/mtd0
        loaderdev=mtd0
        loaderaddr=0xe8000000
        jffs2dev=
        jffs2image=
        swi=switchlight-powerpc.swi
        flash2dev=/dev/mmcblk0
        flash2fsdev=/dev/mmcblk0p1
        ;;
    dni_7448)
        flashimage=switchlight.delta-7448.flash
        flashdev=/dev/mtd0
        loaderdev=mtd0
        loaderaddr=0xe8000000
        jffs2dev=
        jffs2image=
        swi=switchlight-powerpc.swi
        flash2dev=/dev/mmcblk0
        flash2fsdev=/dev/mmcblk0p1
        ;;
    celestica-redstone)
        flashimage=switchlight.celestica-redstone.flash
        flashdev=/dev/mtd0
        loaderdev=mtd0
        loaderaddr=0xec000000
        jffs2dev=
        jffs2image=
        swi=switchlight-powerpc.swi
        flash2dev=
        flash2fsdev=
        ;;
    powerpc-es5652bt1-flf-zz-r0)
	blockinstall=/dev/sda
	partition1size=16M
	partition2size=64M
	partition3size=
	loaderimage=switchlight.accton-es5652bt.loader
	swi=switchlight-powerpc.swi
	platform_bootcmd='usb start; fatload usb 0:1 0x10000000 switchlight-loader; setenv bootargs console=\$consoledev,\$baudrate ; bootm 0x10000000'
	bootconfig='default'
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
sed -e '1,/^PAYLOAD_FOLLOWS$/d' "$0" | gzip -dc | ( cd /tmp/.installer && cpio -imdv ) || exit 1

set -e


say "Installing Switch Light loader (takes several minutes)"

if [ "$blockinstall" ]; then
    # Standard 3 partition formatting
    echo -e "o\nn\np\n1\n\n+${partition1size}\nn\np\n2\n\n+${partition2size}\nn\np\n3\n\n${partition3size}\np\nw\n" | fdisk ${blockinstall}
    mkdosfs /dev/sda1
    mkdosfs /dev/sda2
    mkdosfs /dev/sda3
    mkdir -p /tmp/.fs
    mount /dev/sda1 /tmp/.fs
    cp /tmp/.installer/${loaderimage} /tmp/.fs/switchlight-loader
    umount /tmp/.fs
    if [ "$swi" ]; then
        say "Installing Switch Light software image"
	mount /dev/sda3 /tmp/.fs
        cp /tmp/.installer/${swi} /tmp/.fs/.ztn-switchlight.swi
	umount /tmp/.fs
    fi
    if [ "$bootconfig" ]; then
	say "Installing persistent /mnt/flash"
	mount /dev/sda2 /tmp/.fs
	if [ -f "/tmp/installer/${bootconfig}" ]; then
	    cp /tmp/installer/${bootconfig} /tmp/.fs/boot-config
	else
	    if [ "${bootconfig}" = "default" ]; then
		bootconfig='SWI=flash2:.ztn-switchlight.swi\nNETDEV=ma1\nNETAUTO=dhcp\n'
	    fi
	    echo -e "${bootconfig}" > /tmp/.fs/boot-config
	fi
	umount /dev/sda2
    fi	  
fi
      
if [ "$flashimage" ]; then
    flashcp -v /tmp/.installer/${flashimage} ${flashdev}
fi

if [ "$jffs2dev" ]; then
    if [ -e "$jffs2dev" ]; then
        say "Installing persistent /mnt/flash (takes several minutes)"
        if [ -d "/tmp/.installer/$jffs2image" ]; then
            flash_erase -j ${jffs2dev} 0 0
            mkdir /tmp/.installer/mnt
            mount -t jffs2 ${jffs2dev} /tmp/.installer/mnt
            cp -a /tmp/.installer/$jffs2image/. /tmp/.installer/mnt/.
        else
            flashcp -v /tmp/.installer/${jffs2image} ${jffs2dev}
        fi
    else
        say "Device $jffs2dev invalid, cannot stage /mnt/flash"
        exit 1
    fi
fi

if [ "${flash2dev}" ]; then
    if blockdev --getsize64 "$flash2dev" 1>/dev/null 2>&1; then
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
        cp /tmp/.installer/${swi} /tmp/.fs/.ztn-switchlight.swi
        umount /tmp/.fs
    else
        say "Flash device ${flash2dev} not found; skipping software image install"
    fi
fi

if [ "${platform_bootcmd}" ]; then
    bootcmd=${platform_bootcmd}
else
    bootcmd="setenv bootargs console=\$consoledev,\$baudrate sl_loader=${loaderdev}; bootm ${loaderaddr}"
fi

installer_md5=$(md5sum "$0" | awk '{print $1}')
if [ -f "$0.url" ]; then
    installer_url=$(cat "$0.url")
fi

if [ "${onie_platform}" ]; then
    say "Setting boot command to boot Switch Light"
    cp /dev/null /tmp/.env
    echo "nos_bootcmd ${bootcmd}" >>/tmp/.env
    echo "sl_installer_md5 ${installer_md5}" >>/tmp/.env
    echo "sl_installer_version ${sl_version}" >> /tmp/.env
    if [ "$installer_url" ]; then
        echo "sl_installer_url ${installer_url}" >> /tmp/.env
    else
        echo "sl_installer_url" >> /tmp/.env
    fi
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
