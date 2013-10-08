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

ifndef ROOTFS_ARCH_REPO_NAME
ROOTFS_ARCH_REPO_NAME := repo.$(ROOTFS_ARCH)
endif

ifndef ROOTFS_ARCH_REPO_PATH
ROOTFS_ARCH_REPO_PATH := $(ROOTFS_BUILD_DIR)/$(ROOTFS_ARCH_REPO_NAME)
endif

ifndef ROOTFS_ALL_REPO_NAME
ROOTFS_ALL_REPO_NAME := repo.all
endif

ifndef ROOTFS_ALL_REPO_PATH
ROOTFS_ALL_REPO_PATH := $(ROOTFS_BUILD_DIR)/$(ROOTFS_ALL_REPO_NAME)
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
	arch_repo=$$(mktemp); echo $$arch_repo; sed "s%__DIR__%$(SWITCHLIGHT_REPO)%g" $(ROOTFS_ARCH_REPO_PATH) >$$arch_repo; all_repo=$$(mktemp); sed "s%__DIR__%$(SWITCHLIGHT_REPO)%g" $(ROOTFS_ALL_REPO_PATH) >$$all_repo; $(SWITCHLIGHT)/tools/mkws --apt-cache $(APT_CACHE) --nested -a $(ROOTFS_ARCH) --extra-repo $$arch_repo --extra-repo $$all_repo --extra-config $(ROOTFS_CLEANUP_PATH) $(ROOTFS_DIR) 
	touch $@

$(ROOTFS_DIR).sqsh: $(ROOTFS_BUILD_DIR)/.$(ROOTFS_NAME).done
	f=$$(mktemp); sudo mksquashfs $(ROOTFS_DIR) $$f -no-progress -noappend -comp xz && sudo cat $$f > $(ROOTFS_DIR).sqsh

$(ROOTFS_DIR).cpio: $(ROOTFS_BUILD_DIR)/.$(ROOTFS_NAME).done
	sudo -- /bin/sh -c "cd $(ROOTFS_DIR); find . -print0 | cpio -0 -H newc -o" > $@

$(ROOTFS_NAME).sqsh: $(ROOTFS_DIR).sqsh

$(ROOTFS_NAME).cpio: $(ROOTFS_DIR).cpio

clean:
	$(SL_V_at)sudo rm -rf $(ROOTFS_DIR)
	$(SL_V_at)rm -f $(ROOTFS_DIR).sqsh
	$(SL_V_at)rm -f $(ROOTFS_DIR).cpio
	$(SL_V_at)rm -f $(ROOTFS_BUILD_DIR)/.$(ROOTFS_NAME).done
