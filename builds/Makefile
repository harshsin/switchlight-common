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

############################################################
#
# SWI Builds
#
############################################################

# These are the available SWI architectures
SWI_ARCHS := $(filter-out swi/all,$(wildcard swi/*))

#
# These are the available platform builds per-architecture
# SWI_BUILD_PATHS := $(foreach arch,$(SWI_ARCHS),$(wildcard $(arch)/*))
#
# These are the current default subset
SWI_BUILD_PATHS := \
	swi/powerpc/internal \
	swi/powerpc/release \
	swi/powerpc/bringup \
	swi/powerpc/internal-t5
#swi/i386/vm-internal

#
# Define a build template for each platform by name and build directory
#

define swi_build_template
swi-$(1):
	$(MAKE) -C $(2)
	mkdir -p $(BUILDDIR)
	cp $(2)/*20*.swi $(BUILDDIR)
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
	cp $(2)/*.installer $(BUILDDIR)
	$(MAKE) -C $(2) clean
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

PARALLEL_0 := \
kernel-85xx:powerpc \
kernel-as-e500mc:powerpc \
initrd-powerpc:powerpc

# kernel-x86-64:amd64 initrd-i386:i386 kernel-e500mc:powerpc
$(foreach c,$(PARALLEL_0),$(eval $(call cbuild,$(c),submodules_init)))

PARALLEL_1 := \
platform-powerpc-quanta-lb9a-r0:powerpc \
platform-powerpc-quanta-lb9-r0:powerpc \
platform-powerpc-quanta-ly2-r0:powerpc \
platform-powerpc-as6700-32x-r0:powerpc \
platform-powerpc-as5710-54x-r0a:powerpc
# loader-i386:i386
$(foreach c,$(PARALLEL_1),$(eval $(call cbuild,$(c),$(PARALLEL_0))))

PARALLEL_2 := \
libbroadcom-6.3-internal:powerpc \
libbroadcom-6.3-release:powerpc \
libbroadcom-6.3-internal-src:all \
libbroadcom-6.3-kernel-85xx-modules:powerpc \
libbroadcom-6.3-kernel-as-e500mc-modules:powerpc

$(foreach c,$(PARALLEL_2),$(eval $(call cbuild,$(c),$(PARALLEL_1))))

PARALLEL_3 := \
faultd:powerpc \
sofcon:powerpc \
brcmd-6.3.3-internal:powerpc \
ofad-6.3.3-release:powerpc \
ofad-6.3.3-internal:powerpc \
ofad-6.3.3-t5:powerpc \
orc:powerpc \
orc-brcm:powerpc \
pcli:all \
slrest:all \
switchlight-internal-dbg-sources:all

# faultd:amd64 brcmd-6.3.3-sim:amd64 ofad-6.3.3-sim:amd64 \

$(foreach c,$(PARALLEL_3),$(eval $(call cbuild,$(c),$(PARALLEL_2))))

parallel0: $(subst :,.,$(PARALLEL_0))
parallel1: $(subst :,.,$(PARALLEL_1))
parallel2: $(subst :,.,$(PARALLEL_2))
parallel3: $(subst :,.,$(PARALLEL_3))



