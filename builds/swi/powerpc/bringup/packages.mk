############################################################
#
#
############################################################
SWI_PLATFORMS := powerpc-quanta-lb9-r0 \
		 powerpc-quanta-lb9a-r0 \
		 powerpc-quanta-ly2-r0 \
		 powerpc-as6700-32x-r0 \
		 powerpc-as5710-54x-r0a \
		 powerpc-as5710-54x-r0b \
		 powerpc-as4600-54t \
		 powerpc-as5610-52x \
		 powerpc-dni-7448-r0


include $(SWITCHLIGHT)/builds/swi/powerpc/basepackages.mk

SWI_PACKAGES_POWERPC += brcmd-6.3.3-internal \
			brcmd-6.3.3-internal-dbg \
			libbroadcom-6.3-internal-dbg \
			libbroadcom-6.3-kernel-85xx-modules \
			libbroadcom-6.3-kernel-as-e500mc-modules \
			sofcon \
			gdb \
			binutils \
			file







