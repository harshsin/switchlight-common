############################################################
#
#
############################################################
include $(SWITCHLIGHT)/builds/swi/amd64/basepackages.mk
include platforms.mk

SWI_PACKAGES_ALL += switchlight-internal-dbg-sources

SWI_PACKAGES_amd64 +=   gdb \
			binutils \
			file
ifndef NOTYET
SWI_PACKAGES_amd64 +=   brcmd-6.3.3-internal \
			brcmd-6.3.3-internal-dbg \
			libbroadcom-6.3-internal-dbg \
			libbroadcom-6.3-kernel-x86-64-modules:amd64
endif







