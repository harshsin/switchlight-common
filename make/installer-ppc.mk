###############################################################################
#
# Common Powerpc Installers
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

# Get the platform loaders from each platform package
PLATFORM_LOADERS := $(foreach p,$(INSTALLER_PLATFORMS),$(shell $(SWITCHLIGHT_PKG_INSTALL) platform-$(p):powerpc --find-file switchlight.$(p).loader))
# Get the platform config package for each platform

shar installer $(INSTALLER_NAME).shar: $(PLATFORM_DIRS) $(INSTALLER_SWI)
	$(SL_V_at)cp $(PLATFORM_LOADERS) .
	$(foreach p,$(INSTALLER_PLATFORMS), $(SWITCHLIGHT_PKG_INSTALL) platform-config-$(p):all --extract .;)
	$(SL_V_at)cp $(INSTALLER_SWI) switchlight-powerpc.swi
	$(SL_V_at)sed \
	  -e 's^@SLVERSION@^$(RELEASE)^g' \
	  $(SWITCHLIGHT)/builds/installer/installer.sh \
	>> installer.sh
	$(SL_V_GEN)set -o pipefail ;\
	if $(SL_V_P); then v="-v"; else v="--quiet"; fi ;\
	$(SWITCHLIGHT)/tools/mkshar --lazy $@ $(SWITCHLIGHT)/tools/sfx.sh.in installer.sh *.loader lib switchlight-powerpc.swi
	$(SL_V_at)rm -f switchlight-powerpc.swi installer.sh
	$(SL_V_at)rm -rf ./lib ./usr *.loader

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
	rm -f *.jffs2 *.loader *.swi *.installer
