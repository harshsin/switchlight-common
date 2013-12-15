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

# Get the platform dir from each platform package
PLATFORM_DIRS := $(foreach p,$(INSTALLER_PLATFORMS),$(shell $(SWITCHLIGHT_PKG_INSTALL) platform-$p:powerpc --find-dir platforms))

$(INSTALLER_NAME): $(INSTALLER_NAME).cpio
	$(SL_V_at)cp /dev/null $@
	$(SL_V_at)sed \
	  -e 's^@SLVERSION@^$(RELEASE)^g' \
	  $(SWITCHLIGHT)/builds/installer/installer2.sh \
	>> $@
	$(SL_V_GEN)gzip -9 < $@.cpio >> $@
	$(SL_V_at)rm $@.cpio

$(INSTALLER_NAME).cpio: $(PLATFORM_DIRS) $(INSTALLER_SWI)
	$(foreach dir,$(PLATFORM_DIRS), cp -R $(dir) .;)
	$(SL_V_at)cp $(INSTALLER_SWI) switchlight-powerpc.swi
	$(SL_V_GEN)set -o pipefail ;\
	if $(SL_V_P); then v="-v"; else v="--quiet"; fi ;\
	find platforms switchlight-powerpc.swi \
	| cpio $$v -H newc -o > $@
	$(SL_V_at)rm -f switchlight-powerpc.swi
	$(SL_V_at)rm -rf platforms

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
