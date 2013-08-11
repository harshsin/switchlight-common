###############################################################################
#
# Common rootfs build rules. 
#
###############################################################################

ifndef SWITCHLIGHT
$(error $$SWITCHLIGHT not defined.)
endif

ifndef ROOTFS_BUILD_DIR
$(error $$ROOTFS_BUILD_DIR is not specified.)
endif

ifndef ROOTFS_ARCH
$(error $$ROOTFS_ARCH is not specified.)
endif

include $(SWITCHLIGHT)/make/config.mk
include $(SWITCHLIGHT)/make/packages.mk

ROOTFS_NAME=rootfs-$(ROOTFS_ARCH)
ROOTFS_DIR=$(ROOTFS_BUILD_DIR)/$(ROOTFS_NAME)

ifndef ROOTFS_REPO_NAME
ROOTFS_REPO_NAME := repo
endif

ifndef ROOTFS_REPO_PATH
ROOTFS_REPO_PATH := $(ROOTFS_BUILD_DIR)/$(ROOTFS_REPO_NAME)
endif

ifndef ROOTFS_CLEANUP_NAME
ROOTFS_CLEANUP_NAME := cleanup
endif

ifndef ROOTFS_CLEANUP_PATH
ROOTFS_CLEANUP_PATH := $(ROOTFS_BUILD_DIR)/$(ROOTFS_CLEANUP_NAME)
endif

rootfs.all: $(ROOTFS_DIR).sqsh $(ROOTFS_DIR).cpio

export SWITCHLIGHT

ifndef APT_CACHE
APT_CACHE := 10.198.0.0:3142/
endif

$(ROOTFS_BUILD_DIR)/.$(ROOTFS_NAME).done: $(SWITCHLIGHT_PACKAGE_MANIFEST)
	sudo update-binfmts --enable
	sudo rm -rf $(ROOTFS_DIR)
	f=$$(mktemp); sed "s%__DIR__%$(SWITCHLIGHT_REPO)%g" $(ROOTFS_REPO_PATH) >$$f; $(SWITCHLIGHT)/tools/mkws --apt-cache $(APT_CACHE) --nested -a $(ROOTFS_ARCH) --extra-repo $$f \
--extra-config $(ROOTFS_CLEANUP_PATH) $(ROOTFS_DIR)
	touch $@

$(ROOTFS_DIR).sqsh: $(ROOTFS_BUILD_DIR)/.$(ROOTFS_NAME).done
	f=$$(mktemp); sudo mksquashfs $(ROOTFS_DIR) $$f -no-progress -noappend -comp xz && sudo cat $$f > $(ROOTFS_DIR).sqsh

$(ROOTFS_DIR).cpio: $(ROOTFS_BUILD_DIR)/.$(ROOTFS_NAME).done
	sudo -- /bin/sh -c "cd $(ROOTFS_DIR); find . -print0 | cpio -0 -H newc -o" > $@

$(ROOTFS_NAME).sqsh: $(ROOTFS_DIR).sqsh

$(ROOTFS_NAME).cpio: $(ROOTFS_DIR).cpio

clean:
	@sudo rm -rf $(ROOTFS_DIR)
	@rm -f $(ROOTFS_DIR).sqsh
	@rm -f $(ROOTFS_DIR).cpio
	@rm -f $(ROOTFS_BUILD_DIR)/.$(ROOTFS_NAME).done
