###############################################################################
# 
# THE MTD INSTALLER IS DEPRECATED AND NO LONGER USED. 
$(error THIS INSTALLER IS DEPRECATED.)
#
###############################################################################


ifndef SWITCHLIGHT
$(error $$SWITCHLIGHT is not set.)
else
include $(SWITCHLIGHT)/make/config.mk
endif

# The platform list must be specified
ifndef INSTALLER_PLATFORMS
$(error $$INSTALLER_PLATFORMS not defined)
endif
# The SWI to include in the installer must be specified
ifndef INSTALLER_SWI
$(error $$INSTALLER_SWI is not set)
endif

# The final name of the installer file must be specified. 
ifndef INSTALLER_NAME
$(error $$INSTALLER_NAME is not set)
endif

# Each platform flash template comes from the platform debian package:
# (not every platform includes every file)
PLATFORM_FLASH_TEMPLATES := $(foreach p,$(INSTALLER_PLATFORMS),$(shell $(SWITCHLIGHT_PKG_INSTALL) platform-$p:powerpc --find-file switchlight.$(p).loader) $(shell $(SWITCHLIGHT_PKG_INSTALL) platform-$p:powerpc --find-file switchlight.$(p).flash) $(shell $(SWITCHLIGHT_PKG_INSTALL) platform-$p:powerpc --find-file switchlight.$(p).jffs2))

# These are the basenames of the flash templates. 
PLATFORM_FLASH_TEMPLATE_NAMES := $(foreach p,$(PLATFORM_FLASH_TEMPLATES),$(notdir $(p)))

$(INSTALLER_NAME): $(INSTALLER_NAME).cpio
	$(SL_V_at)cp /dev/null $@
	$(SL_V_at)sed \
	  -e 's^@SLVERSION@^$(RELEASE)^g' \
	  $(SWITCHLIGHT)/builds/installer/installer.sh \
	>> $@
	$(SL_V_GEN)gzip -9 < $@.cpio >> $@
	$(SL_V_at)rm $@.cpio

$(INSTALLER_NAME).cpio: $(PLATFORM_FLASH_TEMPLATES) $(INSTALLER_SWI)
	$(SL_V_at)cp $(PLATFORM_FLASH_TEMPLATES) .
	$(SL_V_at)cp $(INSTALLER_SWI) switchlight-powerpc.swi
	$(SL_V_GEN)set -o pipefail ;\
	if $(SL_V_P); then v="-v"; else v="--quiet"; fi ;\
	find $(PLATFORM_FLASH_TEMPLATE_NAMES) switchlight-powerpc.swi \
	| cpio $$v -H newc -o > $@
	$(SL_V_at)rm -f *.jffs2 *.loader *.flash
	$(SL_V_at)rm -f switchlight-powerpc.swi

$(INSTALLER_SWI):
	$(MAKE) -C $(dir $(INSTALLER_SWI))

# Build config
ifndef SWITCHLIGHT_BUILD_CONFIG
$(error $$SWITCHLIGHT_BUILD_CONFIG is not defined.)
endif

# Release string, copied from swi.mk
ifndef RELEASE
RELEASE := SwitchLight$(SWITCHLIGHT_RELEASE_BANNER)($(SWITCHLIGHT_BUILD_CONFIG),$(SWITCHLIGHT_BUILD_TIMESTAMP),$(SWITCHLIGHT_BUILD_SHA1))
endif

clean:
	rm -f *.cpio *.jffs2 *.loader *.swi *.installer
