############################################################
#
#
############################################################
include $(SWITCHLIGHT)/builds/swi/powerpc/basepackages.mk

include platforms.mk
SWI_PACKAGES_POWERPC += brcmd-6.3.3-internal \
			brcmd-6.3.3-internal-dbg \
			libbroadcom-6.3-internal-dbg \
			libbroadcom-6.3-kernel-85xx-modules \
			libbroadcom-6.3-kernel-as-e500mc-modules \
			sofcon \
			gdb \
			binutils \
			file







