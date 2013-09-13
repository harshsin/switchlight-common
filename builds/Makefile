###############################################################################
#
# SwitchLight Build Products
#
###############################################################################

.DEFAULT_GOAL = default

# All build products go here
BUILDDIR := BUILDS

###############################################################################
#
# SWI Builds
#
###############################################################################

# These are the available SWI architectures
SWI_ARCHS := $(filter-out swi/all,$(wildcard swi/*))

# These are the available platform builds per-architecture
SWI_BUILD_PATHS := $(foreach arch,$(SWI_ARCHS),$(wildcard $(arch)/*))

#
# Define a build template for each platform by name and build directory
#

define swi_build_template
swi-$(1):
	$(MAKE) -C $(2)
	mkdir -p $(BUILDDIR)
	cp $(2)/*20*.swi $(BUILDDIR)
	$(MAKE) -C $(2) clean
endef

$(foreach plat,$(SWI_BUILD_PATHS),$(eval $(call swi_build_template,$(notdir $(plat)),$(plat))))

# Add to ALL_SWI
ALL_SWI := $(foreach plat,$(SWI_BUILD_PATHS),$(notdir $(plat)))
ALL_TARGETS := $(ALL_TARGETS) $(ALL_SWI)

# Build all SWI
swi-all: $(ALL_SWI)

show_swis:	
	@echo $(SWI_BUILD_PATHS)


###############################################################################
#
# Installer Builds
#
###############################################################################

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
endef

$(foreach inst,$(INST_BUILD_PATHS),$(eval $(call installer_build_template,$(notdir $(inst)),$(inst))))

# Add to ALL_INSTALLER
ALL_INSTALLER := $(foreach inst,$(INST_BUILD_PATHS),$(notdir $(inst)))
ALL_TARGETS := $(ALL_TARGETS) $(ALL_INSTALLER)

# Build all installers
installer-all: $(ALL_INSTALLER)

show_installers:
	@echo $(INST_BUILD_PATHS)



###############################################################################
# 
# Components directory
#
###############################################################################
components:
	$(MAKE) -C components

ALL_TARGETS := $(ALL_TARGETS) components


# All you can eat. 
all: $(ALL_TARGETS)

default: show_swis 


