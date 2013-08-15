###############################################################################
#
# SwitchLight
#
###############################################################################

all:
	@echo "targets:"
	@echo "install-build-deps	Install build dependencies into your local workspace"

install-build-deps:
	echo 'APT::Get::AllowUnauthenticated "true";\nAPT::Get::Assume-Yes "true";' | sudo tee /etc/apt/apt.conf.d/99pandora
	sudo dpkg --add-architecture powerpc
	echo "deb http://emdebian.org/debian/ wheezy main" | sudo tee /etc/apt/sources.list.d/emdebian.list
	sudo apt-get update
	sudo apt-get install gcc-4.7-powerpc-linux-gnu libc6-dev-powerpc-cross gcc make xapt cdbs debhelper pkg-config devscripts bison flex texinfo wget cpio multistrap squashfs-tools zip binfmt-support autoconf automake1.9 autotools-dev libtool apt-file file genisoimage syslinux dosfstools mtools bc python-yaml mtd-utils gcc-4.7-multilib
	f=$$(mktemp); wget -O $$f "https://launchpad.net/ubuntu/+source/qemu/1.4.0+dfsg-1expubuntu3/+build/4336762/+files/qemu-user-static_1.4.0%2Bdfsg-1expubuntu3_amd64.deb" && sudo dpkg -i $$f
	sudo update-alternatives --install /usr/bin/powerpc-linux-gnu-gcc powerpc-linux-gnu-gcc /usr/bin/powerpc-linux-gnu-gcc-4.7 10
	sudo xapt -a powerpc libedit-dev ncurses-dev libsensors4-dev libwrap0-dev libssl-dev libsnmp-dev