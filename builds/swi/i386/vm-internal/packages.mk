############################################################
#
#
############################################################
include $(SWITCHLIGHT)/builds/swi/i386/basepackages.mk
include platforms.mk

SWI_PACKAGES_ALL += pcli slrest switchlight-internal-dbg-sources

SWI_PACKAGES_i386 += ofad-ivd \
		     gdb \
		     binutils \
		     file







