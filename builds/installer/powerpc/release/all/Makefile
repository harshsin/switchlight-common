###############################################################################
#
# Build SwitchLight installer for all supported powerpc platforms. 
#
###############################################################################
ifndef SWITCHLIGHT
$(error $$SWITCHLIGHT is not set.)
else
include $(SWITCHLIGHT)/make/config.mk
endif

INSTALLER_PLATFORMS := quanta-lb9 quanta-ly2 quanta-ly5 delta-7448 celestica-redstone
INSTALLER_SWI := $(SWITCHLIGHT)/builds/swi/powerpc/release/switchlight-powerpc-release.swi
INSTALLER_NAME := switchlight-powerpc-release.all.$(SWITCHLIGHT_BUILD_TIMESTAMP).installer

include $(SWITCHLIGHT)/make/installer-ppc.mk
