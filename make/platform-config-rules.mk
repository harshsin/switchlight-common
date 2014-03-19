# This file is included by the debian/rules file for all platform-config packages.
include $(SWITCHLIGHT)/make/config.mk

DEB_DH_INSTALL_SOURCEDIR=debian/tmp
INSTALL_DIR=$(CURDIR)/$(DEB_DH_INSTALL_SOURCEDIR)

BASEDIR=/lib/platform-config

ifndef BRCM_PLATFORM_NAME
BRCM_PLATFORM_NAME=$(PLATFORM_NAME)
endif

ifndef SWITCHLIGHT_SUBMODULE_BROADCOM
$(error $$SWITCHLIGHT_SUBMODULE_BROADCOM is not set.)
endif

%:
	dh $@

build-arch:
	dh build-arch

clean:
	dh clean

override_dh_auto_install:
	mkdir -p $(INSTALL_DIR)$(BASEDIR)/
	cp -R ../../src $(INSTALL_DIR)$(BASEDIR)/$(PLATFORM_NAME)
ifneq ($(BRCM_PLATFORM_NAME),none)
	cp -R $(SWITCHLIGHT_SUBMODULE_BROADCOM)/Modules/BRCM/module/src/platforms/$(BRCM_PLATFORM_NAME)/sbin/* $(INSTALL_DIR)$(BASEDIR)/$(PLATFORM_NAME)/sbin
endif


