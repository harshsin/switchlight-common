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
SWITCHLIGHT_LINUX_KERNEL=$(SWITCHLIGHT)/debian/installs/powerpc/kernel-85xx-headers/usr/kernels/85xx/src/linux-85xx-headers

$(SWITCHLIGHT_LINUX_KERNEL):
	$(SWITCHLIGHT)/debian/install.py kernel-85xx-headers powerpc

###############################################################################

libbroadcom-$(SDK_CONFIG).so.6.2: $(BUILD_SDK_6_2_7_DIR)/libbroadcom-$(SDK_CONFIG).so.6.2

$(BUILD_SDK_6_2_7_DIR)/libbroadcom-$(SDK_CONFIG).so.6.2: $(SWITCHLIGHT_SDK_6_2_7)/build/switchlight/powerpc/$(SDK_CONFIG)/libbroadcom.so 
	cp $(SWITCHLIGHT_SDK_6_2_7)/build/switchlight/powerpc/$(SDK_CONFIG)/libbroadcom.so $(BUILD_SDK_6_2_7_DIR)/libbroadcom-$(SDK_CONFIG).so.6.2
	cp $(SWITCHLIGHT_SDK_6_2_7)/build/switchlight/powerpc/$(SDK_CONFIG)/linux-kernel-bde.ko $(BUILD_SDK_6_2_7_DIR)/linux-kernel-bde-6.2.ko
	cp $(SWITCHLIGHT_SDK_6_2_7)/build/switchlight/powerpc/$(SDK_CONFIG)/linux-user-bde.ko $(BUILD_SDK_6_2_7_DIR)/linux-user-bde-6.2.ko
	cp $(SWITCHLIGHT_SDK_6_2_7)/build/switchlight/powerpc/$(SDK_CONFIG)/libbroadcom.a $(BUILD_SDK_6_2_7_DIR)/libbroadcom-6.2-$(SDK_CONFIG).a
	cp $(SWITCHLIGHT_SDK_6_2_7)/build/switchlight/powerpc/$(SDK_CONFIG)/version.o $(BUILD_SDK_6_2_7_DIR)/version.o
	ln -sf $(BUILD_SDK_6_2_7_DIR)/libbroadcom-$(SDK_CONFIG).so.6.2 $(BUILD_SDK_6_2_7_DIR)/libbroadcom.so
	mkdir -p headers

$(SWITCHLIGHT_SDK_6_2_7)/build/switchlight/powerpc/$(SDK_CONFIG)/libbroadcom.so: $(SWITCHLIGHT_LINUX_KERNEL)
	$(MAKE) -C $(SWITCHLIGHT_SDK_6_2_7)/systems/switchlight/powerpc/$(SDK_CONFIG) SDK=$(SWITCHLIGHT_SDK_6_2_7) SWITCHLIGHT_LINUX_KERNEL=$(SWITCHLIGHT_LINUX_KERNEL)


.PHONY: deb
deb:
	$(MAKE) -C deb SDK=$(SWITCHLIGHT_SDK_6_2_7)








