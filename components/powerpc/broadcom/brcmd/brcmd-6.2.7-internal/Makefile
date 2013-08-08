###############################################################################
#
#  Makefile
#
#
###############################################################################
ifndef SWITCHLIGHT
$(error $$SWITCHLIGHT is undefined.)
else
include $(SWITCHLIGHT)/make/config.mk
endif

BIGCODE := $(SWITCHLIGHT_BIGCODE)
BUILDER := $(BIGCODE)/indigo/Builder/unix
DEBUG := 1

SDKDEV := $(shell $(SWITCHLIGHT)/debian/install.py libbroadcom-6.2-internal-dev powerpc)
SDK := $(SWITCHLIGHT)/debian/installs/powerpc/libbroadcom-6.2-internal-dev/usr/src/libbroadcom-6.2-internal-dev
SDK_BUILD_TARGET := switchlight-powerpc-internal
LIBBROADCOM_A := libbroadcom-6.2-internal.a
LIBBROADCOM_SO := libbroadcom-internal.so.6.2
include $(BROADCOM)/tools/sdk.mk

GLOBAL_CFLAGS += -DSDK_SIM_CONFIG_INCLUDE_SDK_$(SDK_UVERSION)=1

#
# Default to a local build directory for everything
#
ifndef BUILD_DIR
BUILD_DIR := ./build
endif

ifndef TOOLCHAIN
TOOLCHAIN := powerpc-linux-gnu
endif

MODULE := brcmd-6.2.7-internal
include $(BUILDER)/standardinit.mk

LIBRARY := brcmd-6.2.7-internal-lib
$(LIBRARY)_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/lib.mk


MODULE_DIRS := $(BIGCODE)/Modules $(BIGCODE)/indigo/Modules $(BROADCOM)/Modules

DEPENDMODULES := BRCM BRCMTest OS NSS AIM BigList uCli ELS IOF tagport cintm

#
# Resolve module dependencies
#
include $(BUILDER)/dependmodules.mk


BINARY := brcmd.shared
$(BINARY)_LIBRARIES := $(LIBRARY_TARGETS) 
include $(BUILDER)/bin.mk
$(BINARY)_EXTRA_LINK_LIBS += $(SDK_SHARED_LINK_LIBS)

BINARY := brcmd.static
$(BINARY)_LIBRARIES := $(LIBRARY_TARGETS) 
include $(BUILDER)/bin.mk
$(BINARY)_EXTRA_LINK_LIBS += $(SDK_STATIC_LINK_LIBS)


#
# Now that we've specified all the things we want to build, 
# finalize with target definitions. 
#
include $(BUILDER)/targets.mk

#
# Include UCLI support for all modules by default. 
#
GLOBAL_CFLAGS += $(foreach mod,$(DEPENDMODULES_UPPER),-D$(mod)_CONFIG_INCLUDE_UCLI=1)

GLOBAL_CFLAGS += -DOS_CONFIG_INCLUDE_POSIX=1
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MODULES_INIT=1
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MAIN=1
GLOBAL_CFLAGS += -DBRCM_CONFIG_INCLUDE_MAIN=1
GLOBAL_CFLAGS += -DBRCM_CONFIG_INCLUDE_TESTS=1
GLOBAL_CFLAGS += -DBRCM_CONFIG_INCLUDE_AIM_MAIN=1
GLOBAL_CFLAGS += -DUCLI_CONFIG_INCLUDE_ELS_LOOP=1
GLOBAL_CFLAGS += -DBRCM_CONFIG_INCLUDE_RPC_SERVER=0
GLOBAL_CFLAGS += -DBRCM_CONFIG_INCLUDE_RPC_CLIENT=0
GLOBAL_CFLAGS += -DBRCM_CONFIG_TARGET_LINUX_USER=1
GLOBAL_CFLAGS += -DBRCM_CONFIG_INCLUDE_PLATFORM_LB9=1
GLOBAL_CFLAGS += -DBRCM_CONFIG_INCLUDE_PLATFORM_LB9A=1
GLOBAL_CFLAGS += -DCINT_CONFIG_INCLUDE_STDLIB=1
GLOBAL_LINK_LIBS += -ledit -lncurses 


.PHONY: deb

deb: 
	$(MAKE) -C deb


