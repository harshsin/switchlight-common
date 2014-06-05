############################################################
#
#
############################################################
include $(SWITCHLIGHT)/builds/swi/powerpc/basepackages.mk

# Share the same platform list with the release configuration
include ../release-bt/platforms.mk

SWI_PACKAGES_ALL += pcli slrest switchlight-internal-dbg-sources

SWI_PACKAGES_POWERPC += brcmd-6.3.3-internal \
			brcmd-6.3.3-internal-dbg \
			ofad-6.3.3-internal \
			ofad-6.3.3-internal-dbg \
			libbroadcom-6.3-internal-dbg \
			libbroadcom-6.3-kernel-85xx-modules \
			libbroadcom-6.3-kernel-as-e500mc-modules \
			sofcon \
			orc \
			orc-brcm-6.3.3-internal \
			gdb \
			binutils \
			file







