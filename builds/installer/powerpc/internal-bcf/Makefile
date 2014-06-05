############################################################
# <bsn.cl fy=2013 v=none>
#
#        Copyright 2013, 2014 BigSwitch Networks, Inc.
#
#
#
# </bsn.cl>
############################################################
ifndef SWITCHLIGHT
$(error $$SWITCHLIGHT is undefined.)
endif

#
# These are the local packages that must be build
# for this SWI configuration, based on the required platforms.
#
include platforms.mk
SWITCHLIGHT_REQUIRED_PACKAGES := $(foreach p,$(INSTALLER_PLATFORMS),platform-$(p):powerpc) $(foreach p,$(INSTALLER_PLATFORMS),platform-config-$(p):all)

include $(SWITCHLIGHT)/make/component.mk


