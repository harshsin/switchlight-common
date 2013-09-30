############################################################
#
# ------------------------------
# Do not edit this header. 
# It is used by the SPKG script.
# ------------------------------ 
# Package:faultd
# Architecture:powerpc
# ------------------------------
#
############################################################
export ARCH := powerpc
export TOOLCHAIN := powerpc-linux-gnu
export BUILD_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/build

all:
	$(MAKE) -C $(SWITCHLIGHT)/components/any/faultd

deb:
	$(MAKE) -C $(SWITCHLIGHT)/components/any/faultd deb
