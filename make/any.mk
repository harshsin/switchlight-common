############################################################
#
# Build a component from the architecture-any directory. 
#
############################################################
ifndef ANYDIR
$(error $$ANYDIR must be specified.)
endif

ifndef ARCH
$(error $$ARCH must be specified.)
endif

ifndef TOOLCHAIN
$(error $$TOOLCHAIN must be specified.)
endif

ifndef BUILD_DIR_BASE
# Assume the build directory should be in the parent makefile's directory, 
BUILD_DIR_BASE := $(abspath $(dir $(lastword $(filter-out $(lastword $(MAKEFILE_LIST)),$(MAKEFILE_LIST))))/build)
endif

export ARCH
export TOOLCHAIN
export BUILD_DIR_BASE

TARGET_DIR := $(SWITCHLIGHT)/components/any/$(ANYDIR)

all: 
	$(MAKE) -C $(TARGET_DIR)

deb:
	$(MAKE) -C $(TARGET_DIR) deb


