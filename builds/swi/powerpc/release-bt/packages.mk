############################################################
#
#
############################################################
include $(SWITCHLIGHT)/builds/swi/powerpc/basepackages.mk
include platforms.mk

SWI_PACKAGES_ALL += pcli slrest
SWI_PACKAGES_powerpc += ofad-6.3.3-release \
		libbroadcom-6.3-kernel-85xx-modules \
		libbroadcom-6.3-kernel-e500mc-modules






