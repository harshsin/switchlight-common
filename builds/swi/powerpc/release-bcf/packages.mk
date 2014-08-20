############################################################
#
#
############################################################
include $(SWITCHLIGHT)/builds/swi/powerpc/basepackages.mk
include platforms.mk

SWI_PACKAGES_ALL += pcli slrest t6-dump-flows

SWI_PACKAGES_powerpc += ofad-6.3.3-release-t5 \
		libbroadcom-6.3-kernel-85xx-modules \
		libbroadcom-6.3-kernel-e500mc-modules






