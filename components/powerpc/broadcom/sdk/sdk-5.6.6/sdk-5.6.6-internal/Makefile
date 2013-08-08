###############################################################################
#
###############################################################################
BUILD_SDK_5_6_6_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
SDK_CONFIG=internal

include $(SWITCHLIGHT)/make/config.mk

.DEFAULT_GOAL := libbroadcom-$(SDK_CONFIG).so.5.6

###############################################################################
#
# We need the 85XX header package installed before we can build
#
###############################################################################
SWITCHLIGHT_LINUX_KERNEL=$(SWITCHLIGHT)/debian/installs/powerpc/kernel-85xx-headers/usr/kernels/85xx/src/linux-85xx-headers

$(SWITCHLIGHT_LINUX_KERNEL):
	$(SWITCHLIGHT)/debian/install.py kernel-85xx-headers powerpc

###############################################################################

libbroadcom-$(SDK_CONFIG).so.5.6: $(BUILD_SDK_5_6_6_DIR)/libbroadcom-$(SDK_CONFIG).so.5.6

$(BUILD_SDK_5_6_6_DIR)/libbroadcom-$(SDK_CONFIG).so.5.6: $(SWITCHLIGHT_SDK_5_6_6)/build/switchlight/powerpc/$(SDK_CONFIG)/libbroadcom.so 
	cp $(SWITCHLIGHT_SDK_5_6_6)/build/unix-user/pandora-internal-powerpc-2_6/libbroadcom.so $(BUILD_SDK_5_6_6_DIR)/libbroadcom-$(SDK_CONFIG).so.5.6
	cp $(SWITCHLIGHT_SDK_5_6_6)/build/linux-pandora-internal-powerpc-2_6/pandora-internal-powerpc-2_6/linux-kernel-bde.ko $(BUILD_SDK_5_6_6_DIR)/linux-kernel-bde-5.6.ko
	cp $(SWITCHLIGHT_SDK_5_6_6)/build/linux-pandora-internal-powerpc-2_6/pandora-internal-powerpc-2_6/linux-user-bde.ko $(BUILD_SDK_5_6_6_DIR)/linux-user-bde-5.6.ko
	cp $(SWITCHLIGHT_SDK_5_6_6)/build/unix-user/pandora-internal-powerpc-2_6/libbroadcom.a $(BUILD_SDK_5_6_6_DIR)/libbroadcom-5.6-$(SDK_CONFIG).a
	cp $(SWITCHLIGHT_SDK_5_6_6)/build/unix-user/pandora-internal-powerpc-2_6/version.o $(BUILD_SDK_5_6_6_DIR)/version.o
	ln -sf $(BUILD_SDK_5_6_6_DIR)/libbroadcom-$(SDK_CONFIG).so.5.6 $(BUILD_SDK_5_6_6_DIR)/libbroadcom.so
	mkdir -p headers

$(SWITCHLIGHT_SDK_5_6_6)/build/switchlight/powerpc/$(SDK_CONFIG)/libbroadcom.so: $(SWITCHLIGHT_LINUX_KERNEL)
	$(MAKE) -C $(SWITCHLIGHT_SDK_5_6_6)/systems/linux/user/pandora-internal-powerpc-2_6 SDK=$(SWITCHLIGHT_SDK_5_6_6) PANDORA_LINUX_KERNEL=$(SWITCHLIGHT_LINUX_KERNEL)


.PHONY: deb
deb:
	$(MAKE) -C deb SDK=$(SWITCHLIGHT_SDK_5_6_6)








