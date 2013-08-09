###############################################################################
#
# SDK 6.2.7 
#
###############################################################################
ifndef BUILD_SDK_6_2_7_DIR
$(error BUILD_SDK_6_2_7_DIR must be specified. 
endif
ifndef SDK_CONFIG
$(error SDK_CONFIG must be specified. 
endif

include $(SWITCHLIGHT)/make/config.mk

.DEFAULT_GOAL := libbroadcom-$(SDK_CONFIG).so.6.2

###############################################################################
#
# We need the 85XX header package installed before we can build
#
###############################################################################
SWITCHLIGHT_LINUX_KERNEL=$(shell $(SWITCHLIGHT_PKG_INSTALL) kernel-85xx-headers:powerpc --find-dir linux-85xx-headers)

###############################################################################

libbroadcom-$(SDK_CONFIG).so.6.2: $(BUILD_SDK_6_2_7_DIR)/libbroadcom-$(SDK_CONFIG).so.6.2

$(BUILD_SDK_6_2_7_DIR)/libbroadcom-$(SDK_CONFIG).so.6.2: $(SWITCHLIGHT_SUBMODULE_SDK_6_2_7)/build/switchlight/powerpc/$(SDK_CONFIG)/libbroadcom.so 
	cp $(SWITCHLIGHT_SUBMODULE_SDK_6_2_7)/build/switchlight/powerpc/$(SDK_CONFIG)/libbroadcom.so $(BUILD_SDK_6_2_7_DIR)/libbroadcom-$(SDK_CONFIG).so.6.2
	cp $(SWITCHLIGHT_SUBMODULE_SDK_6_2_7)/build/switchlight/powerpc/$(SDK_CONFIG)/linux-kernel-bde.ko $(BUILD_SDK_6_2_7_DIR)/linux-kernel-bde-6.2.ko
	cp $(SWITCHLIGHT_SUBMODULE_SDK_6_2_7)/build/switchlight/powerpc/$(SDK_CONFIG)/linux-user-bde.ko $(BUILD_SDK_6_2_7_DIR)/linux-user-bde-6.2.ko
	cp $(SWITCHLIGHT_SUBMODULE_SDK_6_2_7)/build/switchlight/powerpc/$(SDK_CONFIG)/libbroadcom.a $(BUILD_SDK_6_2_7_DIR)/libbroadcom-6.2-$(SDK_CONFIG).a
	cp $(SWITCHLIGHT_SUBMODULE_SDK_6_2_7)/build/switchlight/powerpc/$(SDK_CONFIG)/version.o $(BUILD_SDK_6_2_7_DIR)/version.o
	ln -sf $(BUILD_SDK_6_2_7_DIR)/libbroadcom-$(SDK_CONFIG).so.6.2 $(BUILD_SDK_6_2_7_DIR)/libbroadcom.so

$(SWITCHLIGHT_SUBMODULE_SDK_6_2_7)/build/switchlight/powerpc/$(SDK_CONFIG)/libbroadcom.so: 
	$(MAKE) -C $(SWITCHLIGHT_SUBMODULE_SDK_6_2_7)/systems/switchlight/powerpc/$(SDK_CONFIG) SDK=$(SWITCHLIGHT_SUBMODULE_SDK_6_2_7) SWITCHLIGHT_LINUX_KERNEL=$(SWITCHLIGHT_LINUX_KERNEL)


.PHONY: deb
deb:
	$(MAKE) -C deb SDK=$(SWITCHLIGHT_SUBMODULE_SDK_6_2_7)








