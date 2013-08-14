###############################################################################
#
# SwitchLight Configuration
#
###############################################################################

#
# These are the default submodule locations. 
# These allow environment overrides for custom frankenbuilds. 
#
ifndef SWITCHLIGHT_SUBMODULE_LINUX
SWITCHLIGHT_SUBMODULE_LINUX      := $(SWITCHLIGHT)/submodules/linux
SWITCHLIGHT_LOCAL_SUBMODULES += linux
endif

ifndef SWITCHLIGHT_SUBMODULE_BIGCODE
SWITCHLIGHT_SUBMODULE_BIGCODE    := $(SWITCHLIGHT)/submodules/bigcode
SWITCHLIGHT_LOCAL_SUBMODULES += bigcode
endif

ifndef SWITCHLIGHT_SUBMODULE_BROADCOM
SWITCHLIGHT_SUBMODULE_BROADCOM   := $(SWITCHLIGHT)/submodules/broadcom
SWITCHLIGHT_LOCAL_SUBMODULES += broadcom
endif

ifndef SWITCHLIGHT_SUBMODULE_LOADER
SWITCHLIGHT_SUBMODULE_LOADER     := $(SWITCHLIGHT)/submodules/loader
SWITCHLIGHT_LOCAL_SUBMODULES += loader
endif

ifndef SWITCHLIGHT_SUBMODULE_SDK_5_6_6
SWITCHLIGHT_SUBMODULE_SDK_5_6_6  := $(SWITCHLIGHT)/submodules/lb9-sdk-5.6.6
SWITCHLIGHT_LOCAL_SUBMODULES += lb9-sdk-5.6.6
endif

ifndef SWITCHLIGHT_SUBMODULE_SDK_6_2_7  
SWITCHLIGHT_SUBMODULE_SDK_6_2_7  := $(SWITCHLIGHT)/submodules/sdk-6.2.7
SWITCHLIGHT_LOCAL_SUBMODULES += sdk-6.2.7
endif

ifndef SWITCHLIGHT_SUBMODULE_SDK_5_10_0 
SWITCHLIGHT_SUBMODULE_SDK_5_10_0 := $(SWITCHLIGHT)/submodules/ly2-sdk-5.10.0
SWITCHLIGHT_LOCAL_SUBMODULES += ly2-sdk-5.10.0
endif

#
# These are the required derivations from the SWITCHLIGHT settings:
#
export BIGCODE := $(SWITCHLIGHT_SUBMODULE_BIGCODE)
export BROADCOM := $(SWITCHLIGHT_SUBMODULE_BROADCOM)
export BUILDER := $(BIGCODE)/indigo/Builder/unix

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

ifndef SWITCHLIGHT_BUILD_TIMESTAMP
SWITCHLIGHT_BUILD_TIMESTAMP := $(shell date +%Y.%m.%d.%H.%M)
endif

ifndef SWITCHLIGHT_BUILD_SHA1
SWITCHLIGHT_BUILD_SHA1 := $(shell git rev-list HEAD -1)
endif

ifndef SWITCHLIGHT_BUILD_CONFIG
SWITCHLIGHT_BUILD_CONFIG := unknown
endif



