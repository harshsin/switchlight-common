###############################################################################
#
###############################################################################

# All available component packages
SWITCHLIGHT_PACKAGES := $(wildcard $(SWITCHLIGHT_REPO)/*.deb)

# Debian Package Manifest
SWITCHLIGHT_PACKAGE_MANIFEST := $(SWITCHLIGHT_REPO)/Packages

# Rebuild the package manifest whenever the component packages are updated. 
$(SWITCHLIGHT_PACKAGE_MANIFEST): $(SWITCHLIGHT_PACKAGES)
	cd $(SWITCHLIGHT_REPO); dpkg-scanpackages . > Packages