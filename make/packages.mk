###############################################################################
#
###############################################################################

ARCHS := powerpc amd64 i386 all

SWITCHLIGHT_PACKAGES_powerpc := $(wildcard $(SWITCHLIGHT_REPO)/powerpc/*.deb)
SWITCHLIGHT_PACKAGES_amd64   := $(wildcard $(SWITCHLIGHT_REPO)/amd64/*.deb)
SWITCHLIGHT_PACKAGES_i386    := $(wildcard $(SWITCHLIGHT_REPO)/i386/*.deb)
SWITCHLIGHT_PACKAGES_all     := $(wildcard $(SWITCHLIGHT_REPO)/all/*.deb)

SWITCHLIGHT_PACKAGES := $(foreach arch,$(ARCHS), $(SWITCHLIGHT_PACKAGES_$(arch)))

# Debian Package Manifest
SWITCHLIGHT_PACKAGE_MANIFEST := $(foreach arch,$(ARCHS),$(SWITCHLIGHT_REPO)/$(arch)/Packages)

# Rebuild the package manifests whenever the component packages are updated. 
$(SWITCHLIGHT_REPO)/%/Packages: $(SWITCHLIGHT_PACKAGES)
	cd $(dir $@); dpkg-scanpackages . > Packages

all: $(SWITCHLIGHT_PACKAGE_MANIFEST)