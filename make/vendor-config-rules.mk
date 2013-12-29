# This file is included by the debian/rules file for all platform-config packages.
DEB_DH_INSTALL_SOURCEDIR=debian/tmp
INSTALL_DIR=$(CURDIR)/$(DEB_DH_INSTALL_SOURCEDIR)
BASEDIR=/usr/lib/python2.7/dist-packages/switchlight/vendor

%:
	dh $@

build-arch:
	dh build-arch

clean:
	dh clean

override_dh_auto_install:
	mkdir -p $(INSTALL_DIR)$(BASEDIR)/
	cp -R ../../src/python/* $(INSTALL_DIR)$(BASEDIR)


