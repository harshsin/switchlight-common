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



# Each platform flash template comes from the platform debian package:
PLATFORM_FLASH_TEMPLATES := $(foreach p,$(INSTALLER_PLATFORMS),$(shell $(SWITCHLIGHT_PKG_INSTALL) platform-$p:powerpc --find-file switchlight.$(p).flash))

# These are the basenames of the flash templates. 
PLATFORM_FLASH_TEMPLATE_NAMES := $(foreach p,$(INSTALLER_PLATFORMS),switchlight.$(p).flash)


$(INSTALLER_NAME): $(PLATFORM_FLASH_TEMPLATES) $(INSTALLER_SWI)
	cp $(PLATFORM_FLASH_TEMPLATES) .
	cp $(INSTALLER_SWI) switchlight-powerpc.swi
	cat $(SWITCHLIGHT)/builds/installer/installer.sh <(tar zc $(PLATFORM_FLASH_TEMPLATE_NAMES) switchlight-powerpc.swi) > $@
	@rm *.flash
	@rm switchlight-powerpc.swi

$(INSTALLER_SWI):
	$(MAKE) -C $(dir $(INSTALLER_SWI))


clean:
	rm *.installer
