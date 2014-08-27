############################################################
#
#
############################################################
include $(SWITCHLIGHT)/builds/swi/amd64/basepackages.mk
include platforms.mk

SWI_PACKAGES_ALL += pcli slrest switchlight-internal-dbg-sources

SWI_PACKAGES_amd64 +=   ofad-ivd \
			gdb \
			binutils \
			file







