# This file is included by the debian/rules file for all platform-config packages.
DEB_DH_INSTALL_SOURCEDIR=debian/tmp
INSTALL_DIR=$(CURDIR)/$(DEB_DH_INSTALL_SOURCEDIR)

BASEDIR=/lib/platform-config

ifndef BRCM_PLATFORM_NAME
BRCM_PLATFORM_NAME=$(PLATFORM_NAME)
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
	cp -R $(SWITCHLIGHT_SUBMODULE_BROADCOM)/Modules/BRCM/module/src/platforms/$(BRCM_PLATFORM_NAME)/sbin/* $(INSTALL_DIR)$(BASEDIR)/$(PLATFORM_NAME)/sbin



