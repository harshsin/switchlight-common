############################################################
#
# SDK Usermode Libraries
#
############################################################
#
# This makefile provides general rules for building SDK user
# libraries for SwitchLight SDKs.
#
############################################################

ifndef LIBRARY_DIR
# The caller should specify the target directory for the
# resulting libraries.
$(error $$LIBRARY_DIR must be specified.)
endif

ifndef SDK_CONFIG
# This is the usermode configuration (internal, release, etc)
$(error $$SDK_CONFIG must be specified.
endif

ifndef SDK
# This is the actual SDK tree we're building from
$(error $$SDK is not set)
endif

ifndef SDK_LIB_VERSION
# This is the version suffix applied to the resulting libraries.
$(error $$SDK_LIB_VERSION is not set)
endif

include $(SWITCHLIGHT)/make/config.mk

.DEFAULT_GOAL := userlibs

.PHONY: deb userlibs

userlibs:
	$(MAKE) -C $(SDK)/systems/switchlight/amd64/$(SDK_CONFIG) SDK=$(SDK) userlibs SDK_CONFIG_FPIC=1
	cp $(SDK)/build/switchlight/amd64/$(SDK_CONFIG)/libbroadcom.so $(LIBRARY_DIR)/libbroadcom-$(SDK_CONFIG).so.$(SDK_LIB_VERSION)
	cp $(SDK)/build/switchlight/amd64/$(SDK_CONFIG)/libbroadcom.a $(LIBRARY_DIR)/libbroadcom-$(SDK_LIB_VERSION)-$(SDK_CONFIG).a
	cp $(SDK)/build/switchlight/amd64/$(SDK_CONFIG)/version.o $(LIBRARY_DIR)/version.o
	ln -sf $(LIBRARY_DIR)/libbroadcom-$(SDK_CONFIG).so.$(SDK_LIB_VERSION) $(LIBRARY_DIR)/libbroadcom.so

deb:
	$(MAKE) -C deb SDK=$(SDK)








