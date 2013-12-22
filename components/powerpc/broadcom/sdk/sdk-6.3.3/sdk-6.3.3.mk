###############################################################################
#
# SDK 6.3.3
#
###############################################################################
ifndef BUILD_SDK_6_3_3_DIR
$(error BUILD_SDK_6_3_3_DIR must be specified. 
endif
ifndef SDK_CONFIG
$(error SDK_CONFIG must be specified. 
endif

include $(SWITCHLIGHT)/make/config.mk

.DEFAULT_GOAL := sdk

###############################################################################
#
# We need the 85XX header package installed before we can build
#
###############################################################################
SWITCHLIGHT_LINUX_KERNEL=$(shell $(SWITCHLIGHT_PKG_INSTALL) kernel-85xx-headers:powerpc --find-dir linux-85xx-headers)

###############################################################################

.PHONY: deb sdk

sdk:
	$(MAKE) -C $(SWITCHLIGHT_SUBMODULE_SDK_6_3_3)/systems/switchlight/powerpc/$(SDK_CONFIG) SDK=$(SWITCHLIGHT_SUBMODULE_SDK_6_3_3) SWITCHLIGHT_LINUX_KERNEL=$(SWITCHLIGHT_LINUX_KERNEL)
	cp $(SWITCHLIGHT_SUBMODULE_SDK_6_3_3)/build/switchlight/powerpc/$(SDK_CONFIG)/libbroadcom.so $(BUILD_SDK_6_3_3_DIR)/libbroadcom-$(SDK_CONFIG).so.6.3
	cp $(SWITCHLIGHT_SUBMODULE_SDK_6_3_3)/build/switchlight/powerpc/$(SDK_CONFIG)/linux-kernel-bde.ko $(BUILD_SDK_6_3_3_DIR)/linux-kernel-bde-6.3.ko
	cp $(SWITCHLIGHT_SUBMODULE_SDK_6_3_3)/build/switchlight/powerpc/$(SDK_CONFIG)/linux-user-bde.ko $(BUILD_SDK_6_3_3_DIR)/linux-user-bde-6.3.ko
	cp $(SWITCHLIGHT_SUBMODULE_SDK_6_3_3)/build/switchlight/powerpc/$(SDK_CONFIG)/libbroadcom.a $(BUILD_SDK_6_3_3_DIR)/libbroadcom-6.3-$(SDK_CONFIG).a
	cp $(SWITCHLIGHT_SUBMODULE_SDK_6_3_3)/build/switchlight/powerpc/$(SDK_CONFIG)/version.o $(BUILD_SDK_6_3_3_DIR)/version.o
	ln -sf $(BUILD_SDK_6_3_3_DIR)/libbroadcom-$(SDK_CONFIG).so.6.3 $(BUILD_SDK_6_3_3_DIR)/libbroadcom.so

deb:
	$(MAKE) -C deb SDK=$(SWITCHLIGHT_SUBMODULE_SDK_6_3_3)








