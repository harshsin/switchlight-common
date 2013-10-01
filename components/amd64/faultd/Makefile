############################################################
#
# ------------------------------
# Do not edit this header. 
# It is used by the SPKG script.
# ------------------------------ 
# Package:faultd
# Architecture:amd64
# ------------------------------
#
############################################################
export ARCH := amd64
export TOOLCHAIN := x86_64-linux-gnu
export BUILD_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/build

all:
	$(MAKE) -C $(SWITCHLIGHT)/components/any/faultd

deb:
	$(MAKE) -C $(SWITCHLIGHT)/components/any/faultd deb
