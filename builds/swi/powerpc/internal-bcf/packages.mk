############################################################
#
#
############################################################
include $(SWITCHLIGHT)/builds/swi/powerpc/basepackages.mk
include platforms.mk

SWI_PACKAGES_ALL += pcli slrest t6-dump-flows switchlight-internal-dbg-sources

SWI_PACKAGES_POWERPC += \
	brcmd-6.3.3-internal \
	libbroadcom-6.3-internal-dbg \
	libbroadcom-6.3-kernel-85xx-modules \
	libbroadcom-6.3-kernel-e500mc-modules \
	ofad-6.3.3-internal-t5 \
	sofcon \
	orc-brcm-6.3.3-internal \
	gdb \
	binutils \
	file \
	orc





