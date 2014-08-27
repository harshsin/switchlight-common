############################################################
# <bsn.cl fy=2013 v=none>
#
#        Copyright 2013, 2014 BigSwitch Networks, Inc.
#
#
#
# </bsn.cl>
############################################################
#
# SwitchLight Build Products
#
############################################################

.DEFAULT_GOAL = default

# All build products go here
BUILDDIR := BUILDS

# Don't keep local debian files during autobuilds to save
# space
NO_LOCAL_DEBS := 1

############################################################
#
# SWI Builds
#
############################################################

# These are the available SWI architectures
SWI_ARCHS := swi/powerpc swi/amd64
SWI_BUILD_PATHS := $(foreach arch,$(SWI_ARCHS),$(filter-out $(arch)/basepackages.mk,$(wildcard $(arch)/*)))

#
# Define a build template for each platform by name and build directory
#

define swi_build_template
swi-$(1):
	$(MAKE) -C $(2)
	mkdir -p $(BUILDDIR)
	cp $(2)/*20*.swi* $(BUILDDIR)
endef

$(foreach plat,$(SWI_BUILD_PATHS),$(eval $(call swi_build_template,$(notdir $(plat)),$(plat))))

# Add to ALL_SWI
ALL_SWI := $(foreach plat,$(SWI_BUILD_PATHS),swi-$(notdir $(plat)))
ALL_TARGETS := $(ALL_TARGETS) $(ALL_SWI)

# Build all SWI
swi-all: $(ALL_SWI)

show-swis:
	@echo $(SWI_BUILD_PATHS)


############################################################
#
# Installer Builds
#
############################################################

# These are the available installer architectures
INST_ARCHS := $(filter-out installer/all,$(wildcard installer/*))

# These are the available installer builds per-architecture
INST_BUILD_PATHS := $(foreach arch,$(INST_ARCHS),$(wildcard $(arch)/*))

#
# Define a build template for each installer by name and build directory
#
define installer_build_template
installer-$(1):
	$(MAKE) -C $(2)
	mkdir -p $(BUILDDIR)
	cp $(2)/*.installer* $(BUILDDIR)
endef

$(foreach inst,$(INST_BUILD_PATHS),$(eval $(call installer_build_template,$(notdir $(inst)),$(inst))))

# Add to ALL_INSTALLER
ALL_INSTALLER := $(foreach inst,$(INST_BUILD_PATHS),installer-$(notdir $(inst)))
ALL_TARGETS := $(ALL_TARGETS) $(ALL_INSTALLER)

# Build all installers
installer-all: $(ALL_INSTALLER)

show-installers:
	@echo $(INST_BUILD_PATHS)


############################################################
#
# Debian Packages
#
############################################################
define deb_build_template
debian-$(1):
	$(MAKE) -C $(2) deb
	mkdir -p $(BUILDDIR)
	cp $(2)/*.deb $(BUILDDIR)
endef

DEB_BUILD_PATHS := $(wildcard deb/*)
$(foreach d,$(DEB_BUILD_PATHS),$(eval $(call deb_build_template,$(notdir $(d)),$(d))))

############################################################
#
# Components directory
#
############################################################
.PHONY: components

components:
	$(MAKE) -C components

ALL_TARGETS := $(ALL_TARGETS) components


# All you can eat.
all: $(ALL_TARGETS)

default: show-swis


############################################################
#
# Parallel Build Optimizations
#
############################################################
# init all submodules
submodules_init:
	$(SWITCHLIGHT)/tools/submodules.py __all__ __all__ $(SWITCHLIGHT)

define cbuild
$(subst :,.,$(1)): $(subst :,.,$(2))
	$(SWITCHLIGHT)/tools/spkg.py $(1) --build
endef

VENDOR_CONFIGS = $(shell $(SWITCHLIGHT)/tools/spkg.py --list-all | grep vendor-config)
SERIAL_TARGETS = vendor-config-switchlight:all $(VENDOR_CONFIGS) onlp-platform-defaults:powerpc onlp-platform:powerpc onlp:powerpc \
		 onlp-platform-defaults:amd64 onlp-platform:amd64 onlp:amd64
serial0: submodules_init
	$(foreach t,$(SERIAL_TARGETS),$(SWITCHLIGHT)/tools/spkg.py $(t) --build;)



PARALLEL_0 := \
kernel-85xx:powerpc \
kernel-e500mc:powerpc \
initrd-powerpc:powerpc \
kernel-x86-64:amd64 \
initrd-amd64:amd64
$(foreach c,$(PARALLEL_0),$(eval $(call cbuild,$(c),submodules_init)))

PARALLEL_1 := $(shell $(SWITCHLIGHT)/tools/spkg.py --list-all | grep platform-powerpc-) loader-amd64:amd64
$(foreach c,$(PARALLEL_1),$(eval $(call cbuild,$(c),$(PARALLEL_0))))

PARALLEL_2 := \
libbroadcom-6.3-internal:powerpc \
libbroadcom-6.3-release:powerpc \
libbroadcom-6.3-internal-src:all \
libbroadcom-6.3-kernel-85xx-modules:powerpc \
libbroadcom-6.3-kernel-e500mc-modules:powerpc \
$(shell $(SWITCHLIGHT)/tools/spkg.py --list-all --arch powerpc amd64 | grep onlp-)

$(foreach c,$(PARALLEL_2),$(eval $(call cbuild,$(c),$(PARALLEL_1))))

PARALLEL_3 := \
faultd:powerpc \
faultd:amd64 \
sofcon:powerpc \
brcmd-6.3.3-internal:powerpc \
ofad-6.3.3-release:powerpc \
ofad-6.3.3-internal:powerpc \
ofad-6.3.3-internal-t5:powerpc \
ofad-6.3.3-release-t5:powerpc \
ofad-ivd:amd64 \
orc:powerpc \
orc-brcm-6.3.3-internal:powerpc \
orc-brcm-6.3.3-release:powerpc \
pcli:all \
slrest:all \
t6-dump-flows:all \
ztn:all \
switchlight-internal-dbg-sources:all

$(foreach c,$(PARALLEL_3),$(eval $(call cbuild,$(c),$(PARALLEL_2))))

parallel0: $(subst :,.,$(PARALLEL_0))
parallel1: $(subst :,.,$(PARALLEL_1))
parallel2: $(subst :,.,$(PARALLEL_2))
parallel3: $(subst :,.,$(PARALLEL_3))



