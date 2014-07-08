###############################################################################
#
# SwitchLight Configuration
#
###############################################################################
SHELL := /bin/bash
empty:=
space:= $(empty) $(empty)
# The current release branch or number goes here.
SWITCHLIGHT_RELEASE_VERSION := $(shell git rev-parse --short HEAD)
SWITCHLIGHT_RELEASE_BANNER := $(space)$(SWITCHLIGHT_RELEASE_VERSION)$(space)

#
# These are the default submodule locations.
# These allow environment overrides for custom frankenbuilds.
#
SWITCHLIGHT_LOCAL_SUBMODULES := none

ifndef SWITCHLIGHT_SUBMODULE_ONLP
SWITCHLIGHT_SUBMODULE_ONLP      := $(SWITCHLIGHT)/submodules/onlp
SWITCHLIGHT_LOCAL_SUBMODULES += onlp
endif

ifndef SWITCHLIGHT_SUBMODULE_ONLP_ACCTON
SWITCHLIGHT_SUBMODULE_ONLP_ACCTON      := $(SWITCHLIGHT)/submodules/onlp-accton
SWITCHLIGHT_LOCAL_SUBMODULES += onlp-accton
endif

ifndef SWITCHLIGHT_SUBMODULE_ONLP_DNI
SWITCHLIGHT_SUBMODULE_ONLP_DNI      := $(SWITCHLIGHT)/submodules/onlp-dni
SWITCHLIGHT_LOCAL_SUBMODULES += onlp-dni
endif

ifndef SWITCHLIGHT_SUBMODULE_ONLP_QUANTA
SWITCHLIGHT_SUBMODULE_ONLP_QUANTA      := $(SWITCHLIGHT)/submodules/onlp-quanta
SWITCHLIGHT_LOCAL_SUBMODULES += onlp-quanta
endif

ifndef SWITCHLIGHT_SUBMODULE_LINUX
SWITCHLIGHT_SUBMODULE_LINUX      := $(SWITCHLIGHT)/submodules/linux
SWITCHLIGHT_LOCAL_SUBMODULES += linux
endif

ifndef SWITCHLIGHT_SUBMODULE_LINUX_3_8_13_ACCTON
SWITCHLIGHT_SUBMODULE_LINUX_3_8_13_ACCTON := $(SWITCHLIGHT)/submodules/linux-3.8.13-accton
SWITCHLIGHT_LOCAL_SUBMODULES += linux-3.8.13-accton
endif

ifndef SWITCHLIGHT_SUBMODULE_INFRA
SWITCHLIGHT_SUBMODULE_INFRA	:= $(SWITCHLIGHT)/submodules/infra
SWITCHLIGHT_LOCAL_SUBMODULES += infra
endif

ifndef SWITCHLIGHT_SUBMODULE_INDIGO
SWITCHLIGHT_SUBMODULE_INDIGO	:= $(SWITCHLIGHT)/submodules/indigo
SWITCHLIGHT_LOCAL_SUBMODULES += indigo
endif

ifndef SWITCHLIGHT_SUBMODULE_LOXIGEN_ARTIFACTS
SWITCHLIGHT_SUBMODULE_LOXIGEN_ARTIFACTS := $(SWITCHLIGHT)/submodules/loxigen-artifacts
SWITCHLIGHT_LOCAL_SUBMODULES += loxigen-artifacts
endif

ifndef SWITCHLIGHT_SUBMODULE_T6_MISC
SWITCHLIGHT_SUBMODULE_T6_MISC := $(SWITCHLIGHT)/submodules/t6-misc
SWITCHLIGHT_LOCAL_SUBMODULES += t6-misc
endif

ifndef SWITCHLIGHT_SUBMODULE_BIGCODE
SWITCHLIGHT_SUBMODULE_BIGCODE    := $(SWITCHLIGHT)/submodules/bigcode
SWITCHLIGHT_LOCAL_SUBMODULES += bigcode
endif

ifndef SWITCHLIGHT_SUBMODULE_BIGCODE_INTERNAL
SWITCHLIGHT_SUBMODULE_BIGCODE_INTERNAL := $(SWITCHLIGHT)/submodules/bigcode-internal
SWITCHLIGHT_LOCAL_SUBMODULES += bigcode-internal
endif

ifndef SWITCHLIGHT_SUBMODULE_SL
SWITCHLIGHT_SUBMODULE_SL := $(SWITCHLIGHT)/submodules/SL
# Not actually a git submodule so not added to the local submodules variable
endif

ifndef SWITCHLIGHT_SUBMODULE_BROADCOM
SWITCHLIGHT_SUBMODULE_BROADCOM   := $(SWITCHLIGHT)/submodules/broadcom
SWITCHLIGHT_LOCAL_SUBMODULES += broadcom
endif

ifndef SWITCHLIGHT_SUBMODULE_LOADER
SWITCHLIGHT_SUBMODULE_LOADER     := $(SWITCHLIGHT)/submodules/loader
SWITCHLIGHT_LOCAL_SUBMODULES += loader
endif

ifndef SWITCHLIGHT_SUBMODULE_SDK_6_3_3
SWITCHLIGHT_SUBMODULE_SDK_6_3_3  := $(SWITCHLIGHT)/submodules/sdk-6.3.3
SWITCHLIGHT_LOCAL_SUBMODULES += sdk-6.3.3
endif

ifndef SWITCHLIGHT_SUBMODULE_SWITCHLIGHT_COMMON
SWITCHLIGHT_SUBMODULE_SWITCHLIGHT_COMMON := $(SWITCHLIGHT)/submodules/switchlight-common
SWITCHLIGHT_LOCAL_SUBMODULES += switchlight-common
endif

#
# These are the required derivations from the SWITCHLIGHT settings:
#
ifndef BUILDER
export BUILDER := $(SWITCHLIGHT_SUBMODULE_INFRA)/builder/unix
endif

#
# Location of the local package repository
#
SWITCHLIGHT_REPO := $(SWITCHLIGHT)/debian/repo

# Path to package installer
SWITCHLIGHT_PKG_INSTALL := $(SWITCHLIGHT)/tools/spkg.py

#
# Make sure the required local submodules have been updated.
#
ifdef SWITCHLIGHT_REQUIRED_SUBMODULES
space :=
space +=
SWITCHLIGHT_REQUIRED_SUBMODULES := $(subst $(space),:,$(SWITCHLIGHT_REQUIRED_SUBMODULES))
SWITCHLIGHT_LOCAL_SUBMODULES := $(subst $(space),:,$(SWITCHLIGHT_LOCAL_SUBMODULES))
endif

ifdef SWITCHLIGHT_BUILD_CONFIG_FILE
include $(SWITCHLIGHT_BUILD_CONFIG_FILE)
endif

ifndef SWITCHLIGHT_BUILD_TIMESTAMP
SWITCHLIGHT_BUILD_TIMESTAMP := $(shell date +%Y.%m.%d.%H.%M)
endif

ifndef SWITCHLIGHT_BUILD_SHA1
SWITCHLIGHT_BUILD_SHA1 := $(shell git rev-list HEAD -1)
endif

ifndef SWITCHLIGHT_BUILD_CONFIG
SWITCHLIGHT_BUILD_CONFIG := unknown
endif

ifeq ("$(origin V)", "command line")
VERBOSE := $(V)
endif
ifneq ($(VERBOSE),1)

# quiet settings
SL_V_P := false
SL_V_at := @
SL_V_GEN = @set -e; echo GEN $@;

else

# verbose settings
SL_V_P := :
SWITCHLIGHT_PKG_INSTALL := $(SWITCHLIGHT)/tools/spkg.py --verbose

endif

ifeq ($(VERBOSE),1)
SL_MAKEFLAGS += V=1
else
SL_MAKEFLAGS += --no-print-directory
endif

#
# Inherit MODULE_DIRs for all local builds.
# This turns out to  be terribly hacky wrt the component makefiles.
# This should be a temporary solution.
#
ALL_SUBMODULES = INFRA INDIGO BIGCODE BIGCODE_INTERNAL SL SWITCHLIGHT_COMMON ONLP ONLP_ACCTON ONLP_DNI ONLP_QUANTA
MODULE_DIRS := $(foreach submodule,$(ALL_SUBMODULES),$(SWITCHLIGHT_SUBMODULE_$(submodule))/modules) $(SWITCHLIGHT_SUBMODULE_BROADCOM)/Modules
MODULE_DIRS_TOUCH := $(foreach sd,$(MODULE_DIRS),$(shell mkdir -p $(sd) && touch $(sd)/Manifest.mk))

override BROADCOM := $(SWITCHLIGHT_SUBMODULE_BROADCOM)
export BROADCOM
override BIGCODE := $(SWITCHLIGHT_SUBMODULE_BIGCODE)
export BIGCODE
override SUBMODULE_LOXIGEN_ARTIFACTS := $(SWITCHLIGHT_SUBMODULE_LOXIGEN_ARTIFACTS)
export SUBMODULE_LOXIGEN_ARTIFACTS
